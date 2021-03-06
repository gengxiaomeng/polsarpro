/********************************************************************
PolSARpro v5.0 is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License as 
published by the Free Software Foundation; either version 2 (1991) of
the License, or any later version. This program is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. 

See the GNU General Public License (Version 2, 1991) for more details

*********************************************************************

File  : kozlov_anisotropy.c
Project  : ESA_POLSARPRO-SATIM
Authors  : Eric POTTIER, Jacek STRZELCZYK
Version  : 2.0
Creation : 07/2015
Update  :
*--------------------------------------------------------------------
INSTITUT D'ELECTRONIQUE et de TELECOMMUNICATIONS de RENNES (I.E.T.R)
UMR CNRS 6164

Waves and Signal department
SHINE Team 


UNIVERSITY OF RENNES I
B�t. 11D - Campus de Beaulieu
263 Avenue G�n�ral Leclerc
35042 RENNES Cedex
Tel :(+33) 2 23 23 57 63
Fax :(+33) 2 23 23 69 63
e-mail: eric.pottier@univ-rennes1.fr

*--------------------------------------------------------------------

Description :  Kozlov Anisotropy determination

********************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "omp.h"

#ifdef _WIN32
#include <dos.h>
#include <conio.h>
#endif

/* ROUTINES DECLARATION */
#include "../lib/PolSARproLib.h"

/********************************************************************
*********************************************************************
*
*            -- Function : Main
*
*********************************************************************
********************************************************************/
int main(int argc, char *argv[])
{

#define NPolType 5
/* LOCAL VARIABLES */
  FILE *out_file[3];
  int Config;
  char *PolTypeConf[NPolType] = {"S2", "C3", "C4", "T3", "T4"};
  char file_name[FilePathLength];
  
/* Internal variables */
  int ii, lig, col, k;
  int ligDone = 0;

  float Phi, Tau;
  float TT11, TT12_re, TT12_im, TT13_re, TT13_im, TT22, TT23_re, TT23_im, TT33;
  float Exr, Exi, Eyr, Eyi, g0, g1, g2, g3;
  float T11_phi, T13_im_phi;
  float T22_phi, T33_phi, T11_d, T22_d;
//  float T12_re_phi, T12_im_phi, T13_re_phi, T23_re_phi, T23_im_phi

/* Matrix arrays */
  float ***M_in;
  float **M_avg;
  float ***M_out;

  float ***G;
  float ***V;
  float *lambda;

/********************************************************************
********************************************************************/
/* USAGE */

strcpy(UsageHelp,"\nkozlov_anisotropy.exe\n");
strcat(UsageHelp,"\nParameters:\n");
strcat(UsageHelp," (string)	-id  	input directory\n");
strcat(UsageHelp," (string)	-od  	output directory\n");
strcat(UsageHelp," (string)	-iodf	input-output data format\n");
strcat(UsageHelp," (int)   	-nwr 	Nwin Row\n");
strcat(UsageHelp," (int)   	-nwc 	Nwin Col\n");
strcat(UsageHelp," (int)   	-ofr 	Offset Row\n");
strcat(UsageHelp," (int)   	-ofc 	Offset Col\n");
strcat(UsageHelp," (int)   	-fnr 	Final Number of Row\n");
strcat(UsageHelp," (int)   	-fnc 	Final Number of Col\n");
strcat(UsageHelp,"\nOptional Parameters:\n");
strcat(UsageHelp," (string)	-mask	mask file (valid pixels)\n");
strcat(UsageHelp," (string)	-errf	memory error file\n");
strcat(UsageHelp," (noarg) 	-help	displays this message\n");
strcat(UsageHelp," (noarg) 	-data	displays the help concerning Data Format parameter\n");

/********************************************************************
********************************************************************/

strcpy(UsageHelpDataFormat,"\nPolarimetric Input-Output Data Format\n\n");
for (ii=0; ii<NPolType; ii++) CreateUsageHelpDataFormatInput(PolTypeConf[ii]); 
strcat(UsageHelpDataFormat,"\n");

/********************************************************************
********************************************************************/
/* PROGRAM START */

if(get_commandline_prm(argc,argv,"-help",no_cmd_prm,NULL,0,UsageHelp)) {
  printf("\n Usage:\n%s\n",UsageHelp); exit(1);
  }
if(get_commandline_prm(argc,argv,"-data",no_cmd_prm,NULL,0,UsageHelpDataFormat)) {
  printf("\n Usage:\n%s\n",UsageHelpDataFormat); exit(1);
  }

if(argc < 19) {
  edit_error("Not enough input arguments\n Usage:\n",UsageHelp);
  } else {
  get_commandline_prm(argc,argv,"-id",str_cmd_prm,in_dir,1,UsageHelp);
  get_commandline_prm(argc,argv,"-od",str_cmd_prm,out_dir,1,UsageHelp);
  get_commandline_prm(argc,argv,"-iodf",str_cmd_prm,PolType,1,UsageHelp);
  get_commandline_prm(argc,argv,"-nwr",int_cmd_prm,&NwinL,1,UsageHelp);
  get_commandline_prm(argc,argv,"-nwc",int_cmd_prm,&NwinC,1,UsageHelp);
  get_commandline_prm(argc,argv,"-ofr",int_cmd_prm,&Off_lig,1,UsageHelp);
  get_commandline_prm(argc,argv,"-ofc",int_cmd_prm,&Off_col,1,UsageHelp);
  get_commandline_prm(argc,argv,"-fnr",int_cmd_prm,&Sub_Nlig,1,UsageHelp);
  get_commandline_prm(argc,argv,"-fnc",int_cmd_prm,&Sub_Ncol,1,UsageHelp);

  get_commandline_prm(argc,argv,"-errf",str_cmd_prm,file_memerr,0,UsageHelp);

  MemoryAlloc = -1; MemoryAlloc = CheckFreeMemory();
  MemoryAlloc = my_max(MemoryAlloc,1000);

  PSP_Threads = omp_get_max_threads();
  if (PSP_Threads <= 2) {
    PSP_Threads = 1;
    } else {
	PSP_Threads = PSP_Threads - 1;
	}
  omp_set_num_threads(PSP_Threads);

  FlagValid = 0;strcpy(file_valid,"");
  get_commandline_prm(argc,argv,"-mask",str_cmd_prm,file_valid,0,UsageHelp);
  if (strcmp(file_valid,"") != 0) FlagValid = 1;

  Config = 0;
  for (ii=0; ii<NPolType; ii++) if (strcmp(PolTypeConf[ii],PolType) == 0) Config = 1;
  if (Config == 0) edit_error("\nWrong argument in the Polarimetric Data Format\n",UsageHelpDataFormat);
  }

  if (strcmp(PolType,"S2")==0) strcpy(PolType, "S2T3");

/********************************************************************
********************************************************************/

  check_dir(in_dir);
  check_dir(out_dir);
  if (FlagValid == 1) check_file(file_valid);

  NwinLM1S2 = (NwinL - 1) / 2;
  NwinCM1S2 = (NwinC - 1) / 2;

/* INPUT/OUPUT CONFIGURATIONS */
  read_config(in_dir, &Nlig, &Ncol, PolarCase, PolarType);
  
/* POLAR TYPE CONFIGURATION */
  PolTypeConfig(PolType, &NpolarIn, PolTypeIn, &NpolarOut, PolTypeOut, PolarType);
  
  file_name_in = matrix_char(NpolarIn,1024); 

/* INPUT/OUTPUT FILE CONFIGURATION */
  init_file_name(PolTypeIn, in_dir, file_name_in);

/* INPUT FILE OPENING*/
  for (Np = 0; Np < NpolarIn; Np++)
  if ((in_datafile[Np] = fopen(file_name_in[Np], "rb")) == NULL)
    edit_error("Could not open input file : ", file_name_in[Np]);

  if (FlagValid == 1) 
    if ((in_valid = fopen(file_valid, "rb")) == NULL)
      edit_error("Could not open input file : ", file_valid);

/* OUTPUT FILE OPENING*/
  sprintf(file_name, "%s%s", out_dir, "anisotropy_kozlov.bin");
  if ((out_file[0] = fopen(file_name, "wb")) == NULL)
  edit_error("Could not open input file : ", file_name);
  sprintf(file_name, "%s%s", out_dir, "anisotropy_cmplx_kozlov.bin");
  if ((out_file[1] = fopen(file_name, "wb")) == NULL)
  edit_error("Could not open input file : ", file_name);
  sprintf(file_name, "%s%s", out_dir, "anisotropy_cmplx_kozlov_norm.bin");
  if ((out_file[2] = fopen(file_name, "wb")) == NULL)
  edit_error("Could not open input file : ", file_name);

/********************************************************************
********************************************************************/
/* MEMORY ALLOCATION */
/*
MemAlloc = NBlockA*Nlig + NBlockB
*/ 

/* Local Variables */
  NBlockA = 0; NBlockB = 0;
  /* Mask */ 
  NBlockA += Sub_Ncol+NwinC; NBlockB += NwinL*(Sub_Ncol+NwinC);

  /* Mout = 3*Nlig*Sub_Ncol */
  NBlockA += 3*Sub_Ncol; NBlockB += 0;
  /* Min = NpolarOut*Nlig*Sub_Ncol */
  NBlockA += NpolarOut*(Ncol+NwinC); NBlockB += NpolarOut*NwinL*(Ncol+NwinC);
  /* Mavg = NpolarOut */
  NBlockA += 0; NBlockB += NpolarOut*Sub_Ncol;
  
/* Reading Data */
  NBlockB += Ncol + 2*Ncol + NpolarIn*2*Ncol + NpolarOut*NwinL*(Ncol+NwinC);

  memory_alloc(file_memerr, Sub_Nlig, NwinL, &NbBlock, NligBlock, NBlockA, NBlockB, MemoryAlloc);

/********************************************************************
********************************************************************/
/* MATRIX ALLOCATION */

  _VC_in = vector_float(2*Ncol);
  _VF_in = vector_float(Ncol);
  _MC_in = matrix_float(4,2*Ncol);
  _MF_in = matrix3d_float(NpolarOut,NwinL, Ncol+NwinC);

/*-----------------------------------------------------------------*/   

  Valid = matrix_float(NligBlock[0] + NwinL, Sub_Ncol + NwinC);

  M_in = matrix3d_float(NpolarOut, NligBlock[0] + NwinL, Ncol + NwinC);
  //M_avg = matrix_float(NpolarOut, Sub_Ncol);
  M_out = matrix3d_float(3, NligBlock[0], Sub_Ncol);
  
/********************************************************************
********************************************************************/
/* MASK VALID PIXELS (if there is no MaskFile */
  if (FlagValid == 0) 
#pragma omp parallel for private(col)
    for (lig = 0; lig < NligBlock[0] + NwinL; lig++) 
      for (col = 0; col < Sub_Ncol + NwinC; col++) 
        Valid[lig][col] = 1.;
 
/********************************************************************
********************************************************************/
/* DATA PROCESSING */

for (Nb = 0; Nb < NbBlock; Nb++) {
  ligDone = 0;
  if (NbBlock > 2) {printf("%f\r", 100. * Nb / (NbBlock - 1));fflush(stdout);}
 
  if (FlagValid == 1) read_block_matrix_float(in_valid, Valid, Nb, NbBlock, NligBlock[Nb], Sub_Ncol, NwinL, NwinC, Off_lig, Off_col, Ncol);

  if (strcmp(PolType,"S2")==0) {
    read_block_S2_noavg(in_datafile, M_in, PolTypeOut, NpolarOut, Nb, NbBlock, NligBlock[Nb], Sub_Ncol, NwinL, NwinC, Off_lig, Off_col, Ncol);
    } else {
  /* Case of C,T or I */
    read_block_TCI_noavg(in_datafile, M_in, NpolarOut, Nb, NbBlock, NligBlock[Nb], Sub_Ncol, NwinL, NwinC, Off_lig, Off_col, Ncol);
    }
  if (strcmp(PolTypeIn,"C3")==0) C3_to_T3(M_in, NligBlock[Nb], Sub_Ncol + NwinC, 0, 0);
  if (strcmp(PolTypeIn,"C4")==0) C4_to_T3(M_in, NligBlock[Nb], Sub_Ncol + NwinC, 0, 0);
  if (strcmp(PolTypeIn,"T4")==0) T4_to_T3(M_in, NligBlock[Nb], Sub_Ncol + NwinC, 0, 0);


Phi = Tau = TT11 = TT12_re = TT12_im = TT13_re = TT13_im = TT22 = TT23_re = TT23_im = TT33 = 0.;
Exr = Exi = Eyr = Eyi = g0 = g1 = g2 = g3 = T11_phi = T13_im_phi = T22_phi = T33_phi = T11_d = T22_d = 0.;
#pragma omp parallel for private(col, k, G, V, lambda, M_avg) firstprivate(Phi, Tau, TT11, TT12_re, TT12_im, TT13_re, TT13_im, TT22, TT23_re, TT23_im, TT33, Exr, Exi, Eyr, Eyi, g0, g1, g2, g3, T11_phi, T13_im_phi, T22_phi, T33_phi, T11_d, T22_d) shared(ligDone)
  for (lig = 0; lig < NligBlock[Nb]; lig++) {
    ligDone++;
    if (omp_get_thread_num() == 0) PrintfLine(ligDone,NligBlock[Nb]);
    G = matrix3d_float(2, 2, 2);
    V = matrix3d_float(2, 2, 2);
    lambda = vector_float(2);
    M_avg = matrix_float(NpolarOut,Sub_Ncol);
    average_TCI(M_in, Valid, NpolarOut, M_avg, lig, Sub_Ncol, NwinL, NwinC, NwinLM1S2, NwinCM1S2);
    for (col = 0; col < Sub_Ncol; col++) {
      if (Valid[NwinLM1S2+lig][NwinCM1S2+col] == 1.) {
        TT11 = eps + M_avg[0][col];
        TT12_re = eps + M_avg[1][col];
        TT12_im = eps + M_avg[2][col];
        TT13_re = eps + M_avg[3][col];
        TT13_im = eps + M_avg[4][col];
        TT22 = eps + M_avg[5][col];
        TT23_re = eps + M_avg[6][col];
        TT23_im = eps + M_avg[7][col];
        TT33 = eps + M_avg[8][col];

        /* Graves matrix determination*/
        G[0][0][0] = 0.5 * (TT11 + TT22 + TT33) + TT12_re;
        G[0][0][1] = 0.;
        G[1][1][0] = 0.5 * (TT11 + TT22 + TT33) - TT12_re;
        G[1][1][1] = 0.;
        G[0][1][0] = TT13_re;
        G[0][1][1] = -TT23_im;
        G[1][0][0] = TT13_re;
        G[1][0][1] = TT23_im;
    
        /* EIGENVECTOR/EIGENVALUE DECOMPOSITION */
        /* V complex eigenvecor matrix, lambda real vector*/
        Diagonalisation(2, G, V, lambda);

        for (k = 0; k < 2; k++) if (lambda[k] < 0.) lambda[k] = 0.;

        M_out[0][lig][col] = (lambda[0] - lambda[1]) / (lambda[0] + lambda[1] + eps);

        /* Angle Estimation */
        Exr = V[0][0][0]; Exi = V[0][0][1];
        Eyr = V[1][0][0]; Eyi = V[1][0][1];
        g0 = (Exr*Exr+Exi*Exi) + (Eyr*Eyr+Eyi*Eyi);
        g1 = (Exr*Exr+Exi*Exi) - (Eyr*Eyr+Eyi*Eyi);
        g2 = 2.*(Exr*Eyr+Exi*Eyi);
        g3 = -2.*(Exi*Eyr-Exr*Eyi);
        Phi = 0.5*atan2(g2,g1);
        Tau = 0.5*asin(g3/g0);

        /* Coherency Matrix des-orientation */
        /* Real Rotation Phi */
        T11_phi = TT11;
//        T12_re_phi = TT12_re * cos(2 * Phi) + TT13_re * sin(2 * Phi);
//        T12_im_phi = TT12_im * cos(2 * Phi) + TT13_im * sin(2 * Phi);
//        T13_re_phi = -TT12_re * sin(2 * Phi) + TT13_re * cos(2 * Phi);
        T13_im_phi = -TT12_im * sin(2 * Phi) + TT13_im * cos(2 * Phi);
        T22_phi = TT22 * cos(2 * Phi) * cos(2 * Phi) + TT23_re * sin(4 * Phi) + TT33 * sin(2 * Phi) * sin(2 * Phi);
//        T23_re_phi = 0.5 * (TT33 - TT22) * sin(4 * Phi) + TT23_re * cos(4 * Phi);
//        T23_im_phi = TT23_im;
        T33_phi = TT22 * sin(2 * Phi) * sin(2 * Phi) - TT23_re * sin(4 * Phi) + TT33 * cos(2 * Phi) * cos(2 * Phi);
  
        /* Elliptical Rotation Tau */
        T11_d = T11_phi * cos(2 * Tau) * cos(2 * Tau) +  T13_im_phi * sin(4 * Tau);
        T11_d = T11_d + T33_phi * sin(2 * Tau) * sin(2 * Tau);

        T22_d = T22_phi;

        /* Kozlov Complex Anisotropy */
        M_out[1][lig][col] = (T22_d - T11_d)/(T22_d + T11_d + eps);
        M_out[2][lig][col] = (M_out[1][lig][col] + 1.) / 2.;
        } else {
        M_out[0][lig][col] = 0.; M_out[1][lig][col] = 0.; M_out[2][lig][col] = 0.;
        }
      }
    free_matrix3d_float(G, 2, 2);
    free_matrix3d_float(V, 2, 2);
    free_vector_float(lambda);
    free_matrix_float(M_avg,NpolarOut);
    }
  write_block_matrix3d_float(out_file, 3, M_out, NligBlock[Nb], Sub_Ncol, 0, 0, Sub_Ncol);
  } // NbBlock

/********************************************************************
********************************************************************/
/* MATRIX FREE-ALLOCATION */
/*
  free_matrix_float(Valid, NligBlock[0]);

  free_matrix3d_float(M_avg, NpolarOut, NligBlock[0]);
  free_matrix3d_float(M_out, 3, NligBlock[0]);
*/  
/********************************************************************
********************************************************************/
/* INPUT FILE CLOSING*/
  for (Np = 0; Np < NpolarIn; Np++) fclose(in_datafile[Np]);
  if (FlagValid == 1) fclose(in_valid);

/* OUTPUT FILE CLOSING*/
  for (Np = 0; Np < 3; Np++) fclose(out_file[Np]);
  
/********************************************************************
********************************************************************/

  return 1;
}


