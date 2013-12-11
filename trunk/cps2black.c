/*
* CPS-II patcher for use with CPS-III hardware
* Artemio Urbina, 2012-2013
* Based on work by DarkSOft and Razoola
*
* The CPS-2 patcher is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* The CPS-2 patcher is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CPS-2 patcher; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA     02111-1307      USA


*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define CPS2_CRC_COMP       0x3FFEFE
#define FILE_50_OFFSET      0x0280A800
#define CPS3_CRC_OFFSET     0x3FEE07
#define CPS3_NAME_OFFSET    0x8028
#define WARZARD_CRC         0x32
#define SF3_CRC             0x6c

unsigned char *ProcessCPS2ROMs(const char *romname);
//void CalculateCRCs(unsigned char *iso, unsigned int type);

void printUsage(char *cmdname)
{
	printf("Usage: %s <-sp> [CPS-III Iso] [CPS2 ROM base name]\n", cmdname);
    printf("\tEx: %s cap-sf3-3.iso mvcj-pnx\n", cmdname);
    printf("\tWould use the mvcj-pnx.03 -> mvcj-pnx.10 ROMs\n");
    printf("\n\tThe optional '-sp' generates a stand alone file for SuperBios usage\n");
}

int main(int argc, char **argv)
{  
  const char *isoname = NULL, *romname = NULL;
  unsigned char *cps2roms = NULL, *modifiediso = NULL, cps3name[9], *isoout;
  unsigned int leniso = 0, lenread = 0, lenwrt = 0, type = 0, lenname = 0;
  FILE *isofile = NULL, *newiso = NULL;
  unsigned long crc = 0, i = 0;
  int superbios = 0;

  printf("-= Black CPS-2 Phoenix tool =-\n\t2012-2013 Artemio Urbina\n\tBased on DarkSoft and Razoola's work\n\tVersion 1.01\n\n");
  if (argc != 3)
  {
	printUsage(argv[0]);
    return 1;
  }

  if(strcmp(argv[1], "-sp") == 0)
  {
	superbios = 1;
	romname = argv[2];
  }
  else	
  {
  	isoname = argv[1];
  	romname = argv[2];
  }	

  if(!superbios)
  {
    isofile = fopen(isoname, "rb");
    if(!isofile)
    {
	  printf("ISO file '%s' could not be read\n", isoname);
	  return 1;
    }
  }

  // Read CPS2 ROMS
  cps2roms = ProcessCPS2ROMs(romname);
  if(!cps2roms)
  {    
    fclose(isofile);
    return 1;
  }
  
  if(superbios)
  {
  	// Save ROM
	lenname = strlen(romname);
	lenname += 8; 
	isoout = (unsigned char *)malloc(lenname); 
	if(!isoout)
	{
	  printf("Could not allocate string\n");	    
	  free(cps2roms);
	  return 1;
	}  
  	sprintf(isoout, "%s-SB.bin", romname);
	newiso = fopen(isoout ,"wb");
	if(!newiso)
	{
	  printf("Could not write ROM\n");	    
	  free(cps2roms);
	  free(isoout);
	  return 1;
	}
	
	lenwrt = fwrite(cps2roms, 1, 0x400000, newiso);
	if(lenwrt != 0x400000)
	{
	  printf("Could not write ROM\n");	    
	  free(cps2roms);
	  free(isoout);
	  return 1;
	}
	
	fclose(newiso);  	  
	printf("* ROM saved correctly as '%s'.\n", isoout);
  	free(isoout);
  	return 1;
  }

  // Read game title from ISO
  fseek(isofile, CPS3_NAME_OFFSET, SEEK_SET); 
  fread(cps3name, 1, 8, isofile);  
  cps3name[8] = '\0';

  // Define CRC values, taken form Darksoft's code
  if(strcmp(cps3name, "CAP-WZD-") == 0)
  {
    printf("* Warzard/Red Earth ISO found.\n");
    type = WARZARD_CRC;
  }
  if(strcmp(cps3name, "CAP-SF3-") == 0)
  {
    printf("* SFIII ISO found.\n");
    type = SF3_CRC;
  }

  if(!type)
  {    
    printf("ISO File must be from Warzard/Red Earth or Street Fighter III\n", cps3name);
    fclose(isofile);
    free(cps2roms);
    return 1;
  }  

  // Load ISO to RAM
  fseek(isofile, 0, SEEK_END);
	leniso = ftell(isofile);
  modifiediso = (unsigned char *)malloc(leniso);
  if(!modifiediso)
  {    
    printf("Could not allocate %s bytes in RAM for the ISO\n", leniso);
    free(cps2roms);    
    fclose(isofile);
    return 1;
  }

  fseek(isofile, 0, SEEK_SET);
	lenread = fread(modifiediso, 1, leniso, isofile);
  if(leniso != lenread)
  {
    printf("Error while loading ISO file\n");
    free(cps2roms);    
    fclose(isofile);
    return 1;
  }
  fclose(isofile);

  printf("* Injecting CPS2 ROM to simm 5 file on CD.\n");
  for(i = 0; i < 0x400000; i++) // 32mbit
  {          
    // SIMM 5 (File 50) offset within ISO
	modifiediso[FILE_50_OFFSET + i] = cps2roms[i]; 
	crc += cps2roms[i];		
  }
  
  //Adjust CPS-III CRC
  modifiediso[FILE_50_OFFSET + CPS3_CRC_OFFSET] += type - (crc & 0xff);          

  //CalculateCRCs(modifiediso, type);   

  // Compensate CPS2 CRC, uncommenting it breaks CPS-III CRC
  //modifiediso[0x400000 - CPS2_CRC_COMP] = 0xFF - modifiediso[FILE_50_OFFSET + CPS3_CRC_OFFSET];  

  // Save ISO
  lenname = strlen(cps3name);
  lenname += 14 + strlen(romname); 
  isoout = (unsigned char *)malloc(lenname); 
  if(!isoout)
  {
    printf("Could not allocate string\n");
    free(modifiediso);
    free(cps2roms);
    return 1;
  }  

  sprintf(isoout, "%sPatched-%s.iso", cps3name, romname);
  newiso = fopen(isoout ,"wb");
  if(!newiso)
  {
    printf("Could not write patched ISO\n");
    free(modifiediso);
    free(cps2roms);
    free(isoout);
    return 1;
  }

  lenwrt = fwrite(modifiediso, 1, leniso, newiso);
  if(lenwrt != leniso)
  {
    printf("Could not write patched ISO\n");
    free(modifiediso);
    free(cps2roms);
    free(isoout);
    return 1;
  }

  fclose(newiso);  

  free(modifiediso);
  free(cps2roms);

  printf("* ISO patched correctly and saved as '%s'.\n", isoout);

  free(isoout);
  return 0;
}

unsigned char *ProcessCPS2ROMs(const char *romname)
{
  unsigned int lenroms = 0, lenread = 0, lenname = 0, i;
  unsigned char *cps2roms = NULL, *romfilename = NULL;
  FILE *file = NULL;
  
  lenname = strlen(romname);
  lenname += 4; // ".01" -> ".10" \0
  romfilename = (unsigned char *)malloc(lenname); 
  if(!romfilename)
  {
    printf("Could not allocate string\n");
    return NULL;
  }  

  cps2roms = (unsigned char *)malloc(0x400000); // 32 mbit 
  if(!cps2roms)
  {    
    printf("Could not allocate 32 mbit space for roms\n");    
    return NULL;
  }

  memset(cps2roms, 0xFF, 0x400000); 

  printf("* Loading CPS2 roms.\n");
  for(i = 1; i <= 8; i++)
  {
    sprintf(romfilename, "%s.%0.2d", romname, i + 2);
    file = fopen(romfilename, "rb");
	  if (!file) 
    {
      printf("CPS-2 rom file '%s' could not be read", romfilename);
      if(i < 3)
      {
        printf("\n");
        free(romfilename);
        free(cps2roms);
        return NULL;
      }
      else
        printf(", filling with 0xFF\n");      
	}
    else
    {
      fseek(file, 0, SEEK_END);
	  lenroms = ftell(file);

      fseek(file, 0, SEEK_SET);
	  lenread = fread(cps2roms + (0x80000 * (i - 1)), 1, lenroms, file);
      if(lenread != lenroms)
      {
        printf("Error while reading CPS2 roms:\n"); 
        printf("\tRead 0x%X bytes from %s, expected 0x%X\n", lenread, romfilename, lenroms);
        free(romfilename);
        free(cps2roms);
        fclose(file);    
        return NULL;
      }
      fclose(file);
    }
  }
  
  printf("* Processing CPS2 roms.\n");
  // Byte swap CPS2 roms
  i = 0;
  while(i < 0x400000)
  {
    int c, d;

    c = cps2roms[i];
    d = cps2roms[i + 1];

    cps2roms[i] = d;
    cps2roms[i + 1] = c;
    
    i += 2;
  }

  // Code below is for CRC compensation assuming the last bytes are 0xFF and unused

  /*
  // Check if the last 0x101 bytes of the file are free to mess with for CRC
  for(i = (0x400000 - 1); i >= (0x400000 - 0x102); i --)
  {
    if(cps2roms[i] != 0xFF)
    {
        printf("Not enough free space in CPS2 ROM for checksum change\n");
        free(romfilename);
        free(cps2roms);
        return NULL;
    }
    else
        cps2roms[i] = 0x00;
  }
  // We need one more, and compensated the single unit
  // This keeps CRC 0x10000 lower, thus maintaining the same 4 byte checksum
  cps2roms[i + 1] = 0xFE;  
  //cps2roms[CPS2_CRC_COMP] = 0xFF;  

  // Prepare byte for CPS-3 CRC
  if(cps2roms[CPS3_CRC_OFFSET] != 0xFF)
    printf("WARNING. CPS-III CRC byte was 0x%x, expected 0xFF\n", cps2roms[CPS3_CRC_OFFSET]);
  //cps2roms[CPS3_CRC_OFFSET] = 0x00;
  */
  free(romfilename);
  return cps2roms;  
}

/*
void CalculateCRCs(unsigned char *iso, unsigned int type)
{   
  unsigned long int i = 0, crc = 0;
  
  i = crc = 0;
  for(i = 0; i < 0x400000; i++) // 32mbit   
	crc += iso[FILE_50_OFFSET + i];		
      
  //printf("Calculated ISO CRC is 0x%X\n", crc);
  //Adjust CRC
  iso[FILE_50_OFFSET + CPS3_CRC_OFFSET] += type - (crc & 0xff);        
  //printf("Saved ISO CRC 0x%X to address 0x%X\n", iso[FILE_50_OFFSET + CPS3_CRC_OFFSET], FILE_50_OFFSET + CPS3_CRC_OFFSET);  

  // Compensate CPS2 CRC
  iso[FILE_50_OFFSET + CPS2_CRC_COMP] = 0xFF - iso[FILE_50_OFFSET + CPS3_CRC_OFFSET];
  //printf("Compensated CPS2 CRC. Wrote 0x%X to address 0x%X\n", iso[FILE_50_OFFSET + CPS2_CRC_COMP], FILE_50_OFFSET + CPS2_CRC_COMP);      
}
*/