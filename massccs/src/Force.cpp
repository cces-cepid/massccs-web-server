/*
 * This program is licensed granted by STATE UNIVERSITY OF CAMPINAS - UNICAMP ("University")
 * for use of MassCCS software ("the Software") through this website
 * https://github.com/cces-cepid/MassCCS (the "Website").
 *
 * By downloading the Software through the Website, you (the "License") are confirming that you agree
 * that your use of the Software is subject to the academic license terms.
 *
 * For more information about MassCCS please contact: 
 * skaf@unicamp.br (Munir S. Skaf)
 * guido@unicamp.br (Guido Araujo)
 * samuelcm@unicamp.br (Samuel Cajahuaringa)
 * danielzc@unicamp.br (Daniel L. Z. Caetano)
 * zanottol@unicamp.br (Leandro N. Zanotto)
 */


#include "headers/Force.h"

Force::Force(MoleculeTarget *moleculeTarget, LinkedCell *linkedcell, double lj_cutoff, double alpha, double coul_cutoff) {
this->moleculeTarget = moleculeTarget; 
this->linkedcell = linkedcell;           
this->lj_cutoff = lj_cutoff;                     
this->alpha = alpha;
this->coul_cutoff = coul_cutoff;
}

Force::~Force(){	
}

/*
 * Compute the lennard jones force and potential using linked-cell
 */
void Force::lennardjones_LC(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy, fz, U, Ulj, flj, Ulj_cut;    
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double r2, r;
double r2inv, r6inv, rc6inv;
double lj1, lj2, lj3, lj4;
double s1, s2;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;
    
int index; 
linkedcell->calculateIndex(r_probe,index); 
  
if (index >= linkedcell->Ncells || index < 0) {
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
} 	    

int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;  
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];  
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);

    if (r < lj_cutoff) {
      epsilon = moleculeTarget->eps[target_id]; 
      sigma = moleculeTarget->sig[target_id]; 
  
      r2inv = 1.0/r2;
      r6inv = r2inv*r2inv*r2inv;
      lj1 = 4.0*epsilon*pow(sigma,6.0);
      lj2 = lj1*pow(sigma,6.0);
      Ulj = r6inv*(lj2*r6inv - lj1);
      rc6inv = 1.0/pow(lj_cutoff,6);    
      Ulj_cut = rc6inv*(lj2*rc6inv - lj1); 
      lj3 = 6.0*lj1;
      lj4 = 12.0*lj2;
      flj = r6inv*(lj4*r6inv - lj3)*r2inv;

      U += Ulj - Ulj_cut;
      fx += flj*dx;
      fy += flj*dy;
      fz += flj*dz;
    }
  }	    
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = U;
return;
}

/*
 * Compute the lennard jones force and potential
 */

void Force::lennardjones(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj;
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double r2,r;
double r2inv,r6inv;
double lj1,lj2,lj3,lj4;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];

Ulj = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r = sqrt(r2);

  epsilon = moleculeTarget->eps[i];
  sigma = moleculeTarget->sig[i];
     
  r2inv = 1.0/r2;
  r6inv = r2inv*r2inv*r2inv;
  lj1 = 4.0*epsilon*pow(sigma,6.0);
  lj2 = lj1*pow(sigma,6.0);
  Ulj += r6inv*(lj2*r6inv - lj1);

  lj3 = 6.0*lj1;
  lj4 = 12.0*lj2;
  flj = r6inv*(lj4*r6inv - lj3)*r2inv;
  fx += flj*dx;
  fy += flj*dy;
  fz += flj*dz;  
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = Ulj;

return;
}

/**
 * Compute the lennard jones and coulomb interactions
 **/
void Force::lennardjones_coulomb(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj, U;
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double r2, r;
double rinv, r2inv, r6inv;
double lj1, lj2, lj3, lj4;
double qi, qj;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;

double Ucoul, fcoul, s1, s2;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r =  sqrt(r2);

  epsilon = moleculeTarget->eps[i];
  sigma = moleculeTarget->sig[i];
     
  r2inv = 1.0/r2; 
  r6inv = r2inv*r2inv*r2inv;
  lj1 = 4.0*epsilon*pow(sigma,6.0);
  lj2 = lj1*pow(sigma,6.0);
  Ulj = r6inv*(lj2*r6inv - lj1);
  lj3 = 6.0*lj1;
  lj4 = 12.0*lj2;
  flj = r6inv*(lj4*r6inv - lj3)*r2inv;
  
  U += Ulj;
  fx += flj*dx;
  fy += flj*dy;
  fz += flj*dz;

  qj = moleculeTarget->q[i];
  rinv = 1.0/r;
  Ucoul = qi*qj*rinv*KCOUL;
  r2inv = 1.0/r2;
  fcoul = Ucoul*r2inv; 
  U += Ucoul;
  fx += fcoul*dx;
  fy += fcoul*dy;
  fz += fcoul*dz; 
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = U;

return;
}

/**
 * Compute the coulomb interactions
 **/
void Force::coulomb(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz;
double dx, dy, dz;
double r2,r;
double rinv,r2inv;
double Ucoul, fcoul, U;
double qi, qj;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

fx = 0.0;
fy = 0.0;
fz = 0.0;
U = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r = sqrt(r2);
  
  qj = moleculeTarget->q[i];
  rinv = 1.0/r;
  Ucoul = qi*qj*rinv*KCOUL;
  r2inv = 1.0/r2;
  fcoul = Ucoul*r2inv;

  U += Ucoul;
  fx += fcoul*dx;
  fy += fcoul*dy;
  fz += fcoul*dz;
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = U;

return;
}

/**
 * Compute the lennard jones and induced dipole interactions using linked-cell (apply for Helium)
 **/
void Force::lennardjones_induced_dipole_LC(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj, U, Ulj_cut;
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double r2, x2, y2, z2, r;
double r2inv, r6inv, rc6inv;
double lj1, lj2, lj3, lj4;
double q, qr3inv, qr5inv, qrc;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double r3inv, r5inv, r7inv, r9inv;
double rc3inv, smooth_factor;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;

int index;
linkedcell->calculateIndex(r_probe,index);

if (index >= linkedcell->Ncells || index < 0) {
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
}

Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;

double s1, s2, Exi, Eyi, Ezi, Exxi, Eyyi, Ezzi, Exyi, Exzi, Eyzi;

// calculation lennard-jones and induced dipole interactions on the first neighbors cells
int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);
    r2inv = 1.0/r2;
    
    // lennard-jones interaction
    if (r < lj_cutoff) {
     epsilon = moleculeTarget->eps[target_id];
     sigma = moleculeTarget->sig[target_id];
     r6inv = r2inv*r2inv*r2inv;
     lj1 = 4.0*epsilon*pow(sigma,6.0);
     lj2 = lj1*pow(sigma,6.0);
     Ulj = r6inv*(lj2*r6inv - lj1);
     rc6inv = 1.0/pow(lj_cutoff,6);
     Ulj_cut = rc6inv*(lj2*rc6inv - lj1);

     lj3 = 6.0*lj1;
     lj4 = 12.0*lj2;
     flj = r6inv*(lj4*r6inv - lj3)*r2inv;
       
     U += Ulj - Ulj_cut;
     fx += flj*dx;
     fy += flj*dy;
     fz += flj*dz;
    }

    // ion-induced dipole interaction
    if (r < coul_cutoff) {
     r3inv = 1.0/r*r2inv;
     r5inv = r3inv*r2inv;
     q = moleculeTarget->q[target_id];
     rc3inv = 1.0/pow(coul_cutoff,3);
     smooth_factor = (1.0 - r*r2*rc3inv);
     qr3inv = q*r3inv*smooth_factor;
     qr5inv = -3.0*q*r5inv*smooth_factor;
     qrc = -3.0*q*rc3inv*r2inv; 

     Exi = dx * qr3inv;
     Eyi = dy * qr3inv;
     Ezi = dz * qr3inv;

     Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
     Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
     Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

     Exyi = dx*dy*qr5inv + dx*dy*qrc;
     Exzi = dx*dz*qr5inv + dx*dz*qrc;
     Eyzi = dy*dz*qr5inv + dy*dz*qrc;

     Ex += Exi;
     Ey += Eyi;
     Ez += Ezi;

     Exx += Exxi;
     Eyy += Eyyi;
     Ezz += Ezzi;

     Exy += Exyi;
     Exz += Exzi;
     Eyz += Eyzi;
    }
  }
}

// calculation induced dipole interaction on the second neighbors cells
neighborscells = linkedcell->neighbors2_cells[index];
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors2_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);
    r2inv = 1.0/r2;

    // ion-induced dipole interaction
    if (r < coul_cutoff) {
     r3inv = 1.0/r*r2inv;
     r5inv = r3inv*r2inv;
     q = moleculeTarget->q[target_id];
     rc3inv = 1.0/pow(coul_cutoff,3);
     smooth_factor = (1.0 - r*r2*rc3inv);
     qr3inv = q*r3inv*smooth_factor;
     qr5inv = -3.0*q*r5inv*smooth_factor;
     qrc = -3.0*q*rc3inv*r2inv;

     Exi = dx * qr3inv;
     Eyi = dy * qr3inv;
     Ezi = dz * qr3inv;

     Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
     Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
     Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

     Exyi = dx*dy*qr5inv + dx*dy*qrc;
     Exzi = dx*dz*qr5inv + dx*dz*qrc;
     Eyzi = dy*dz*qr5inv + dy*dz*qrc;

     Ex += Exi;
     Ey += Eyi;
     Ez += Ezi;

     Exx += Exxi;
     Eyy += Eyyi;
     Ezz += Ezzi;

     Exy += Exyi;
     Exz += Exzi;
     Eyz += Eyzi;
    }
  }
}

f[0] = fx + alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
f[1] = fy + alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
f[2] = fz + alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

Up = U - 0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
return;
}

/*
 * Compute the lennard jones and induced dipole interactions (Hellium atom)
 */

void Force::lennardjones_induced_dipole(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj, U;
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double r2, x2, y2, z2, r;
double r2inv, r6inv;
double lj1, lj2, lj3, lj4;
double q, qr3inv, qr5inv;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double r3inv, r5inv, r7inv, r9inv;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];

Ulj = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r =  sqrt(r2);

  epsilon = moleculeTarget->eps[i];
  sigma = moleculeTarget->sig[i];

  r2inv = 1.0/r2;
  r6inv = r2inv*r2inv*r2inv;
  lj1 = 4.0*epsilon*pow(sigma,6.0);
  lj2 = lj1*pow(sigma,6.0);
  Ulj += r6inv*(lj2*r6inv - lj1);
  
  lj3 = 6.0*lj1;
  lj4 = 12.0*lj2;
  flj = r6inv*(lj4*r6inv - lj3)*r2inv;
  fx += flj*dx;
  fy += flj*dy;
  fz += flj*dz;

  q =  moleculeTarget->q[i];
  r3inv = 1.0/r*r2inv;
  r5inv = r3inv*r2inv;
  qr3inv = q*r3inv;
  qr5inv = q*r5inv;

  Ex += dx * qr3inv;
  Ey += dy * qr3inv;
  Ez += dz * qr3inv;

  Exx += qr3inv - 3.0*dx*dx*qr5inv;
  Eyy += qr3inv - 3.0*dy*dy*qr5inv;
  Ezz += qr3inv - 3.0*dz*dz*qr5inv;

  Exy += - 3.0 * dx * dy * qr5inv;
  Exz += - 3.0 * dx * dz * qr5inv;
  Eyz += - 3.0 * dy * dz * qr5inv;
}

f[0] = fx + alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
f[1] = fy + alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
f[2] = fz + alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

Up = Ulj - 0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
return;
}

/*
 * Compute the lennard jones and coulomb interactions using linked-cell (<- N of N2 molecule)
 */
void Force::lennardjones_coulomb_LC(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj, U;
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double r2,r;
double rinv,r2inv,r6inv;
double lj1,lj2,lj3,lj4;
double qi, qj;
double Ucoul, fcoul, s1, s2;
int index;
double Ulj_cut, rc6inv, Ucoul_shift;


r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;

linkedcell->calculateIndex(r_probe,index);

if (index >= linkedcell->Ncells || index < 0) {
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
}

int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);

    if (r < lj_cutoff) {
      epsilon = moleculeTarget->eps[target_id];
      sigma = moleculeTarget->sig[target_id]; 
      r2inv = 1.0/r2;
      r6inv = r2inv*r2inv*r2inv;
      lj1 = 4.0*epsilon*pow(sigma,6.0);
      lj2 = lj1*pow(sigma,6.0);
      Ulj = r6inv*(lj2*r6inv - lj1);
      rc6inv = 1.0/pow(lj_cutoff,6);
      Ulj_cut = rc6inv*(lj2*rc6inv - lj1);
      lj3 = 6.0*lj1;
      lj4 = 12.0*lj2;
      flj = r6inv*(lj4*r6inv - lj3)*r2inv;		

      U += Ulj - Ulj_cut;
      fx += flj*dx;
      fy += flj*dy;
      fz += flj*dz;
    }

    if (r < coul_cutoff) {
      qj = moleculeTarget->q[target_id];
      rinv = 1.0/r;
      Ucoul = qi*qj*rinv*KCOUL;
      Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
      r2inv = 1.0/r2;
      fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));
        
      U += Ucoul + Ucoul_shift;
      fx += fcoul*dx;
      fy += fcoul*dy;
      fz += fcoul*dz; 
    } 	      
  }
}

// calculation induced dipole interaction on the second neighbors cells
neighborscells = linkedcell->neighbors2_cells[index];
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors2_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);

    if (r < coul_cutoff) {
     qj = moleculeTarget->q[target_id];
     rinv = 1.0/r;
     Ucoul = qi*qj*rinv*KCOUL;
     Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
     r2inv = 1.0/r2;
     fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

     U += Ucoul + Ucoul_shift;
     fx += fcoul*dx;
     fy += fcoul*dy;
     fz += fcoul*dz;
    }
  }
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = U;

return;
}

/**
 * Compute coulomb interaction using linked-cell (<- central charge of N2 molecule)
 **/
void Force::coulomb_LC(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy, fz, Ulj, flj, U;
double dx, dy, dz;
double r2, r;
double rinv, r2inv, r6inv;
double lj1, lj2, lj3, lj4;
double qi, qj;
double Ucoul_shift;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;

double Ucoul, fcoul, s1, s2;

int index;
linkedcell->calculateIndex(r_probe,index);

if (index >= linkedcell->Ncells && index < 0) {
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
}

int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);

    if (r < coul_cutoff) {
     qj = moleculeTarget->q[target_id];
     rinv = 1.0/r;
     Ucoul = qi*qj*rinv*KCOUL;
     Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
     r2inv = 1.0/r2;
     fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

     U += Ucoul + Ucoul_shift;
     fx += fcoul*dx;
     fy += fcoul*dy;
     fz += fcoul*dz; 
    } 	      
  }
}

// calculation induced dipole interaction on the second neighbors cells
neighborscells = linkedcell->neighbors2_cells[index];
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors2_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);

    if (r < coul_cutoff) {
     qj = moleculeTarget->q[target_id];
     rinv = 1.0/r;
     Ucoul = qi*qj*rinv*KCOUL;
     Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
     r2inv = 1.0/r2;
     fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

     U += Ucoul + Ucoul_shift;
     fx += fcoul*dx;
     fy += fcoul*dy;
     fz += fcoul*dz;
    }
  }
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = U;

return;
}

/*
 * Compute the coulomb and induced dipole interactions with anisotropy polarizability (<- central charge of N2 molecule)
 */

void Force::coulomb_induced_dipole_iso(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, U;
double dx, dy, dz;
double r2, x2, y2, z2, r;
double r2inv, r6inv;
double fcoul, Ucoul;
double qi, qj, qr3inv, qr5inv, qrc;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double rinv, r3inv, r5inv, r7inv, r9inv, rc3inv;
double U_ind, f_ind[3];

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r =  sqrt(r2);

  qj = moleculeTarget->q[i];
  rinv = 1.0/r;
  Ucoul = qi*qj*rinv*KCOUL;
  r2inv = 1.0/r2;
  fcoul = Ucoul*r2inv;

  U += Ucoul;
  fx += fcoul*dx;
  fy += fcoul*dy;
  fz += fcoul*dz;

  r3inv = rinv*r2inv;
  r5inv = r3inv*r2inv;
  qr3inv = qj*r3inv;
  qr5inv = qj*r5inv;

  Ex += dx * qr3inv;
  Ey += dy * qr3inv;
  Ez += dz * qr3inv;

  Exx += qr3inv - 3.0*dx*dx*qr5inv;
  Eyy += qr3inv - 3.0*dy*dy*qr5inv;
  Ezz += qr3inv - 3.0*dz*dz*qr5inv;

  Exy += - 3.0 * dx * dy * qr5inv;
  Exz += - 3.0 * dx * dz * qr5inv;
  Eyz += - 3.0 * dy * dz * qr5inv;
}

f[0] = fx + alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
f[1] = fy + alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
f[2] = fz + alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

Up = U - 0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
return;
}

/*
 * Compute the coulomb and induced dipole interactions with isotropy polarizability using linked-cell list (<- central charge of N2 molecule)
 */

void Force::coulomb_induced_dipole_iso_LC(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, U;
double dx, dy, dz;
double r2, x2, y2, z2, r;
double r2inv, r6inv, rinv;
double qi, qj, qr3inv, qr5inv, qrc;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double Exi, Eyi, Ezi, Exxi, Exyi, Exzi, Eyyi, Eyzi, Ezzi;
double r3inv, r5inv, r7inv, r9inv, rc3inv;
double Ucoul, fcoul, Ucoul_shift;
double smooth_factor;
double U_ind, f_ind[3];

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

int index;
linkedcell->calculateIndex(r_probe,index);

if (index >= linkedcell->Ncells || index < 0) {
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
}

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;

int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);

    if (r < coul_cutoff) {
     qj = moleculeTarget->q[target_id];
     rinv = 1.0/r;
     Ucoul = qi*qj*rinv*KCOUL;
     Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
     r2inv = 1.0/r2;
     fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

     U += Ucoul + Ucoul_shift;
     fx += fcoul*dx;
     fy += fcoul*dy;
     fz += fcoul*dz; 

     r3inv = rinv*r2inv;
     r5inv = r3inv*r2inv;
     rc3inv = 1.0/pow(coul_cutoff,3);
     smooth_factor = (1.0 - r*r2*rc3inv);
     qr3inv = qj*r3inv*smooth_factor;
     qr5inv = -3.0*qj*r5inv*smooth_factor;
     qrc = -3.0*qj*rc3inv*r2inv; 

     Exi = dx * qr3inv;
     Eyi = dy * qr3inv;
     Ezi = dz * qr3inv;

     Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
     Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
     Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

     Exyi = dx*dy*qr5inv + dx*dy*qrc;
     Exzi = dx*dz*qr5inv + dx*dz*qrc;
     Eyzi = dy*dz*qr5inv + dy*dz*qrc;

     Ex += Exi;
     Ey += Eyi;
     Ez += Ezi;

     Exx += Exxi;
     Eyy += Eyyi;
     Ezz += Ezzi;

     Exy += Exyi;
     Exz += Exzi;
     Eyz += Eyzi;

    } 	      
  }
}

// calculation induced dipole interaction on the second neighbors cells
neighborscells = linkedcell->neighbors2_cells[index];
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors2_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);

    if (r < coul_cutoff) {
     qj = moleculeTarget->q[target_id];
     rinv = 1.0/r;
     Ucoul = qi*qj*rinv*KCOUL;
     Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
     r2inv = 1.0/r2;
     fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

     U += Ucoul + Ucoul_shift;
     fx += fcoul*dx;
     fy += fcoul*dy;
     fz += fcoul*dz;

     r3inv = rinv*r2inv;
     r5inv = r3inv*r2inv;
     rc3inv = 1.0/pow(coul_cutoff,3);
     smooth_factor = (1.0 - r*r2*rc3inv);
     qr3inv = qj*r3inv*smooth_factor;
     qr5inv = -3.0*qj*r5inv*smooth_factor;
     qrc = -3.0*qj*rc3inv*r2inv; 

     Exi = dx * qr3inv;
     Eyi = dy * qr3inv;
     Ezi = dz * qr3inv;

     Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
     Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
     Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

     Exyi = dx*dy*qr5inv + dx*dy*qrc;
     Exzi = dx*dz*qr5inv + dx*dz*qrc;
     Eyzi = dy*dz*qr5inv + dy*dz*qrc;

     Ex += Exi;
     Ey += Eyi;
     Ez += Ezi;

     Exx += Exxi;
     Eyy += Eyyi;
     Ezz += Ezzi;

     Exy += Exyi;
     Exz += Exzi;
     Eyz += Eyzi;
    }
  }
}

f[0] = fx + alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
f[1] = fy + alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
f[2] = fz + alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

Up = U - 0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
return;
}

// only for Carbon of CO2 diatomic molecule

/*
 * Compute the lennard jones force and potential using linked-cell for CO2
 */
void Force::lennardjones_LC_CO2(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy, fz, U, Ulj, flj, Ulj_cut;    
double dx, dy, dz;
double epsilon, sigma;
double r2, r;
double r2inv, r6inv, rc6inv;
double lj1, lj2, lj3, lj4;
double s1, s2;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;
    
int index; 
linkedcell->calculateIndex(r_probe,index); 
  
if (index >= linkedcell->Ncells || index < 0) {  
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
} 	    

int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;  
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];  
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);

    if (r < lj_cutoff) {
      epsilon = moleculeTarget->eps_central[target_id]; 
      sigma = moleculeTarget->sig_central[target_id]; 

      r2inv = 1.0/r2;
      r6inv = r2inv*r2inv*r2inv;
      lj1 = 4.0*epsilon*pow(sigma,6.0);
      lj2 = lj1*pow(sigma,6.0);
      Ulj = r6inv*(lj2*r6inv - lj1);
      rc6inv = 1.0/pow(lj_cutoff,6);    
      Ulj_cut = rc6inv*(lj2*rc6inv - lj1); 
      lj3 = 6.0*lj1;
      lj4 = 12.0*lj2;
      flj = r6inv*(lj4*r6inv - lj3)*r2inv;

      U += Ulj - Ulj_cut;
      fx += flj*dx;
      fy += flj*dy;
      fz += flj*dz;
    }
  }	    
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = U;
return;
}

/*
 * Compute the lennard jones force and potential for CO2
 */

void Force::lennardjones_CO2(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj;
double dx, dy, dz;
double epsilon, sigma;
double r2,r;
double r2inv,r6inv;
double lj1,lj2,lj3,lj4;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];

Ulj = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r = sqrt(r2);

  epsilon = moleculeTarget->eps_central[i]; 
  sigma = moleculeTarget->sig_central[i]; 
     
  r2inv = 1.0/r2;
  r6inv = r2inv*r2inv*r2inv;
  lj1 = 4.0*epsilon*pow(sigma,6.0);
  lj2 = lj1*pow(sigma,6.0);
  Ulj += r6inv*(lj2*r6inv - lj1);

  lj3 = 6.0*lj1;
  lj4 = 12.0*lj2;
  flj = r6inv*(lj4*r6inv - lj3)*r2inv;
  fx += flj*dx;
  fy += flj*dy;
  fz += flj*dz;
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = Ulj;

return;
}

/*
 * Compute the lennard jones and coulomb interactions using linked-cell CO2
 */
void Force::lennardjones_coulomb_LC_CO2(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj, U;
double dx, dy, dz;
double epsilon, sigma;
double r2,r;
double rinv,r2inv,r6inv;
double lj1,lj2,lj3,lj4;
double qi, qj;
double Ucoul, fcoul, s1, s2;
int index;
double Ulj_cut, rc6inv, Ucoul_shift;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;

linkedcell->calculateIndex(r_probe,index);

if (index >= linkedcell->Ncells || index < 0) {
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
}

int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);

    if (r < lj_cutoff) {
      epsilon = moleculeTarget->eps_central[target_id]; 
      sigma = moleculeTarget->sig_central[target_id];      
      r2inv = 1.0/r2;
      r6inv = r2inv*r2inv*r2inv;
      lj1 = 4.0*epsilon*pow(sigma,6.0);
      lj2 = lj1*pow(sigma,6.0);
      Ulj = r6inv*(lj2*r6inv - lj1);
      rc6inv = 1.0/pow(lj_cutoff,6);
      Ulj_cut = rc6inv*(lj2*rc6inv - lj1);
      lj3 = 6.0*lj1;
      lj4 = 12.0*lj2;
      flj = r6inv*(lj4*r6inv - lj3)*r2inv;		

      U += Ulj - Ulj_cut;
      fx += flj*dx;
      fy += flj*dy;
      fz += flj*dz;
    }

    if (r < coul_cutoff) {
      qj = moleculeTarget->q[target_id];
      rinv = 1.0/r;
      Ucoul = qi*qj*rinv*KCOUL;
      Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
      r2inv = 1.0/r2;
      fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));
        
      U += Ucoul + Ucoul_shift;
      fx += fcoul*dx;
      fy += fcoul*dy;
      fz += fcoul*dz; 
    } 	      
  }
}

// calculation coulomb interaction on the second neighbors cells
neighborscells = linkedcell->neighbors2_cells[index];
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors2_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);

    if (r < coul_cutoff) {
      qj = moleculeTarget->q[target_id];
      rinv = 1.0/r;
      Ucoul = qi*qj*rinv*KCOUL;
      Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
      r2inv = 1.0/r2;
      fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

      U += Ucoul + Ucoul_shift;
      fx += fcoul*dx;
      fy += fcoul*dy;
      fz += fcoul*dz;
    }
  }
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = U;

return;
}


/**
 * Compute the lennard jones and coulomb interactions
 **/
void Force::lennardjones_coulomb_CO2(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj, U, Ucoul, fcoul;
double dx, dy, dz;
double epsilon, sigma;
double r2, r;
double rinv, r2inv, r6inv;
double lj1, lj2, lj3, lj4;
double qi, qj;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r =  sqrt(r2);

  epsilon = moleculeTarget->eps_central[i]; 
  sigma = moleculeTarget->sig_central[i]; 
   
  r2inv = 1.0/r2; 
  r6inv = r2inv*r2inv*r2inv;
  lj1 = 4.0*epsilon*pow(sigma,6.0);
  lj2 = lj1*pow(sigma,6.0);
  Ulj = r6inv*(lj2*r6inv - lj1);
  lj3 = 6.0*lj1;
  lj4 = 12.0*lj2;
  flj = r6inv*(lj4*r6inv - lj3)*r2inv;
  
  U += Ulj;
  fx += flj*dx;
  fy += flj*dy;
  fz += flj*dz;
  
  qj = moleculeTarget->q[i];
  rinv = 1.0/r;
  Ucoul = qi*qj*rinv*KCOUL;
  r2inv = 1.0/r2;
  fcoul = Ucoul*r2inv; 
  U += Ucoul;
  fx += fcoul*dx;
  fy += fcoul*dy;
  fz += fcoul*dz; 
}

f[0] = fx;
f[1] = fy;
f[2] = fz;
Up = U;

return;
}

/**
 * Compute the lennard jones, coulomb and induced dipole interactions using linked-cell CO2 (<- C of CO2)
 **/
void Force::lennardjones_coulomb_induced_dipole_iso_LC_CO2(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj, U, Ulj_cut;
double dx, dy, dz;
double epsilon, sigma;
double r2, x2, y2, z2, r;
double r2inv, r6inv, rc6inv;
double lj1, lj2, lj3, lj4;
double q, qr3inv, qr5inv, qrc;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double r3inv, r5inv, r7inv, r9inv;
double rc3inv, smooth_factor;
double rinv;
double Ucoul, Ucoul_shift;
double qi, qj;
double fcoul;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;

int index;
linkedcell->calculateIndex(r_probe,index);

if (index >= linkedcell->Ncells || index < 0) {
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
}

Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;

double s1, s2, Exi, Eyi, Ezi, Exxi, Eyyi, Ezzi, Exyi, Exzi, Eyzi;

// calculation lennard-jones and induced dipole interactions on the first neighbors cells
int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);
    r2inv = 1.0/r2;
    
    // lennard-jones interaction
    if (r < lj_cutoff) {
      epsilon = moleculeTarget->eps_central[target_id]; 
      sigma = moleculeTarget->sig_central[target_id]; 

      r6inv = r2inv*r2inv*r2inv;
      lj1 = 4.0*epsilon*pow(sigma,6.0);
      lj2 = lj1*pow(sigma,6.0);
      Ulj = r6inv*(lj2*r6inv - lj1);
      rc6inv = 1.0/pow(lj_cutoff,6);
      Ulj_cut = rc6inv*(lj2*rc6inv - lj1);

      lj3 = 6.0*lj1;
      lj4 = 12.0*lj2;
      flj = r6inv*(lj4*r6inv - lj3)*r2inv;
        
      U += Ulj - Ulj_cut;
      fx += flj*dx;
      fy += flj*dy;
      fz += flj*dz;
    }

    // coulomb and ion-induced dipole interaction
    if (r < coul_cutoff) {
      qj = moleculeTarget->q[target_id];
      rinv = 1.0/r;
      Ucoul = qi*qj*rinv*KCOUL;
      Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3);
      r2inv = 1.0/r2;
      fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));

      U += Ucoul + Ucoul_shift;
      fx += fcoul*dx;
      fy += fcoul*dy;
      fz += fcoul*dz;

      r3inv = 1.0/r*r2inv;
      r5inv = r3inv*r2inv;
      
      rc3inv = 1.0/pow(coul_cutoff,3);
      smooth_factor = (1.0 - r*r2*rc3inv);
      qr3inv = qj*r3inv*smooth_factor;
      qr5inv = -3.0*qj*r5inv*smooth_factor;
      qrc = -3.0*qj*rc3inv*r2inv; 

      Exi = dx * qr3inv;
      Eyi = dy * qr3inv;
      Ezi = dz * qr3inv;

      Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
      Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
      Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

      Exyi = dx*dy*qr5inv + dx*dy*qrc;
      Exzi = dx*dz*qr5inv + dx*dz*qrc;
      Eyzi = dy*dz*qr5inv + dy*dz*qrc;

      Ex += Exi;
      Ey += Eyi;
      Ez += Ezi;

      Exx += Exxi;
      Eyy += Eyyi;
      Ezz += Ezzi;

      Exy += Exyi;
      Exz += Exzi;
      Eyz += Eyzi;
    }
  }
}

// calculation induced dipole interaction on the second neighbors cells
neighborscells = linkedcell->neighbors2_cells[index];
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors2_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r =  sqrt(r2);
    r2inv = 1.0/r2;

    // ion-induced dipole interaction
    if (r < coul_cutoff) {
      qj = moleculeTarget->q[target_id];
      rinv = 1.0/r;
      Ucoul = qi*qj*rinv*KCOUL;
      Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3);
      r2inv = 1.0/r2;
      fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));

      U += Ucoul + Ucoul_shift;
      fx += fcoul*dx;
      fy += fcoul*dy;
      fz += fcoul*dz;
  
      r3inv = rinv*r2inv;
      r5inv = r3inv*r2inv; 
      rc3inv = 1.0/pow(coul_cutoff,3);
      smooth_factor = (1.0 - r*r2*rc3inv);
      qr3inv = qj*r3inv*smooth_factor;
      qr5inv = -3.0*qj*r5inv*smooth_factor;
      qrc = -3.0*qj*rc3inv*r2inv;

      Exi = dx * qr3inv;
      Eyi = dy * qr3inv;
      Ezi = dz * qr3inv;

      Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
      Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
      Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

      Exyi = dx*dy*qr5inv + dx*dy*qrc;
      Exzi = dx*dz*qr5inv + dx*dz*qrc;
      Eyzi = dy*dz*qr5inv + dy*dz*qrc;

      Ex += Exi;
      Ey += Eyi;
      Ez += Ezi;

      Exx += Exxi;
      Eyy += Eyyi;
      Ezz += Ezzi;

      Exy += Exyi;
      Exz += Exzi;
      Eyz += Eyzi;
    }
  }
}

f[0] = fx + alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
f[1] = fy + alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
f[2] = fz + alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

Up = U - 0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
return;
}

/*
 * Compute the lennard jones, coulomb and induced dipole interactions on CO2
 */

void Force::lennardjones_coulomb_induced_dipole_iso_CO2(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, Ulj, flj, U, Ucoul, fcoul;
double dx, dy, dz;
double epsilon, sigma;
double r2, x2, y2, z2, r;
double r2inv, r6inv;
double lj1, lj2, lj3, lj4;
double qi, qj, qr3inv, qr5inv;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double rinv, r3inv, r5inv, r7inv, r9inv;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r =  sqrt(r2);

  epsilon = moleculeTarget->eps_central[i];
  sigma = moleculeTarget->sig_central[i];

  r2inv = 1.0/r2;
  r6inv = r2inv*r2inv*r2inv;
  lj1 = 4.0*epsilon*pow(sigma,6.0);
  lj2 = lj1*pow(sigma,6.0);
  Ulj = r6inv*(lj2*r6inv - lj1);  
  lj3 = 6.0*lj1;
  lj4 = 12.0*lj2;
  flj = r6inv*(lj4*r6inv - lj3)*r2inv;
  U += Ulj;
  fx += flj*dx;
  fy += flj*dy;
  fz += flj*dz;

  qj = moleculeTarget->q[i];
  rinv = 1.0/r;
  Ucoul = qi*qj*rinv*KCOUL;
  r2inv = 1.0/r2;
  fcoul = Ucoul*r2inv;

  U += Ucoul;
  fx += fcoul*dx;
  fy += fcoul*dy;
  fz += fcoul*dz;

  r3inv = rinv*r2inv;
  r5inv = r3inv*r2inv;
  qr3inv = qj*r3inv;
  qr5inv = qj*r5inv;

  Ex += dx * qr3inv;
  Ey += dy * qr3inv;
  Ez += dz * qr3inv;

  Exx += qr3inv - 3.0*dx*dx*qr5inv;
  Eyy += qr3inv - 3.0*dy*dy*qr5inv;
  Ezz += qr3inv - 3.0*dz*dz*qr5inv;

  Exy += - 3.0 * dx * dy * qr5inv;
  Exz += - 3.0 * dx * dz * qr5inv;
  Eyz += - 3.0 * dy * dz * qr5inv;
}

f[0] = fx + alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
f[1] = fy + alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
f[2] = fz + alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

Up = U - 0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
return;
}

// anisotropy polarizability for induce dipole interaction

/*
 * Compute the coulomb and induced dipole interactions with anisotropy polarizability
 */

void Force::coulomb_induced_dipole_aniso(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, U;
double dx, dy, dz;
double r2, x2, y2, z2, r;
double r2inv, r6inv;
double fcoul, Ucoul;
double qi, qj, qr3inv, qr5inv, qrc;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double rinv, r3inv, r5inv, r7inv, r9inv, rc3inv;
double U_ind, f_ind[3];
double EE[3][3], E[3], alpha_tensor[3][3];
double alpha_d[3][3], RT[3][3], R[3][3];
double alpha_radial, alpha_axial;
double un[3];
double theta, phi;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    alpha_d[i][j] = 0.0;
    alpha_tensor[i][j] = 0.0;
  }
}  

alpha_radial = gas->alpha_radial;
alpha_axial = gas->alpha_axial;

alpha_d[0][0] = alpha_radial;
alpha_d[1][1] = alpha_radial;
alpha_d[2][2] = alpha_axial;

// orientation of molecule
un[0] = gas->x[0] - gas->x[1];
un[1] = gas->y[0] - gas->y[1];
un[2] = gas->z[0] - gas->z[1];

double un2;
un2 = gas->d;

un[0] /= un2;
un[1] /= un2;
un[2] /= un2;

theta = acos(un[2]);
phi = atan2(un[1],un[0]);

R[0][0] = cos(phi)*cos(theta);
R[1][0] = sin(phi)*cos(theta);
R[2][0] = -sin(theta);

R[0][1] = -sin(phi);
R[1][1] = cos(phi);
R[2][1] = 0.0;

R[0][2] = cos(phi)*sin(theta);
R[1][2] = sin(phi)*sin(theta);
R[2][2] = cos(theta);

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    RT[i][j] = R[j][i];
  }
}

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 3; k++) {
      for (int m = 0; m < 3; m++) {
        alpha_tensor[i][j] += R[i][k]*alpha_d[k][m]*RT[m][j];
      }
    }
  }
}

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r =  sqrt(r2);

  qj = moleculeTarget->q[i];
  rinv = 1.0/r;
  Ucoul = qi*qj*rinv*KCOUL;
  r2inv = 1.0/r2;
  fcoul = Ucoul*r2inv;

  U += Ucoul;
  fx += fcoul*dx;
  fy += fcoul*dy;
  fz += fcoul*dz;

  r3inv = rinv*r2inv;
  r5inv = r3inv*r2inv;
  qr3inv = qj*r3inv;
  qr5inv = qj*r5inv;

  Ex += dx * qr3inv;
  Ey += dy * qr3inv;
  Ez += dz * qr3inv;

  Exx += qr3inv - 3.0*dx*dx*qr5inv;
  Eyy += qr3inv - 3.0*dy*dy*qr5inv;
  Ezz += qr3inv - 3.0*dz*dz*qr5inv;

  Exy += - 3.0 * dx * dy * qr5inv;
  Exz += - 3.0 * dx * dz * qr5inv;
  Eyz += - 3.0 * dy * dz * qr5inv;
}

// induced-dipole contribution
E[0] = Ex;
E[1] = Ey;
E[2] = Ez;

EE[0][0] = Exx; //Exx
EE[1][1] = Eyy; //Eyy
EE[2][2] = Ezz; //Ezz

EE[0][1] = Exy; //Exy
EE[1][0] = EE[0][1]; //Eyx
EE[0][2] = Exz; //Exz
EE[2][0] = EE[0][2]; //Ezx
EE[1][2] = Eyz; //Eyz
EE[2][1] = EE[1][2]; //Ezy

U_ind = 0.0;
for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    U_ind += -0.5*alpha_tensor[i][j]*E[j]*E[i];
  }
}

for (int k = 0; k < 3; k++) {
  f_ind[k] = 0.0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      f_ind[k] += 0.5*alpha_tensor[i][j]*(EE[j][k]*E[i] + E[j]*EE[i][k]);
    }
  }
}

f[0] = fx + f_ind[0];
f[1] = fy + f_ind[1];
f[2] = fz + f_ind[2];

Up = U + U_ind;
return;
}

/*
 * Compute the coulomb and induced dipole interactions with anisotropy polarizability using linked-cell list
 */

void Force::coulomb_induced_dipole_aniso_LC(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, U;
double dx, dy, dz;
double r2, x2, y2, z2, r;
double r2inv, r6inv, rinv;
double qi, qj, qr3inv, qr5inv, qrc;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double Exi, Eyi, Ezi, Exxi, Exyi, Exzi, Eyyi, Eyzi, Ezzi;
double r3inv, r5inv, r7inv, r9inv, rc3inv;
double Ucoul, fcoul, Ucoul_shift;
double smooth_factor;
double U_ind, f_ind[3];
double EE[3][3], E[3], alpha_tensor[3][3];
double alpha_d[3][3], RT[3][3], R[3][3];
double theta, phi;
double alpha_axial, alpha_radial, ani_pol;
double un[3];

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

int index;
linkedcell->calculateIndex(r_probe,index);

if (index >= linkedcell->Ncells || index < 0) {
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
}

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    alpha_d[i][j] = 0.0;
    alpha_tensor[i][j] = 0.0;
  }
}  

alpha_radial = gas->alpha_radial;
alpha_axial = gas->alpha_axial;

alpha_d[0][0] = alpha_radial;
alpha_d[1][1] = alpha_radial;
alpha_d[2][2] = alpha_axial;

// orientation of molecule
un[0] = gas->x[0] - gas->x[1];
un[1] = gas->y[0] - gas->y[1];
un[2] = gas->z[0] - gas->z[1];

double un2;
un2 = gas->d;

un[0] /= un2;
un[1] /= un2;
un[2] /= un2;

theta = acos(un[2]);
phi = atan2(un[1],un[0]);

R[0][0] = cos(theta)*cos(phi);
R[1][0] = sin(theta)*cos(phi);
R[2][0] = -sin(phi);

R[0][1] = -sin(theta);
R[1][1] = cos(theta);
R[2][1] = 0.0;

R[0][0] = cos(theta)*sin(phi);
R[1][0] = sin(theta)*sin(phi);
R[2][0] = cos(phi);

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    RT[i][j] = R[j][i];
  }
}

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 3; k++) {
      for (int m = 0; m < 3; m++) {
        alpha_tensor[i][j] += R[i][k]*alpha_d[k][m]*RT[m][j];
      }  
    }
  }
}

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;

int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);

    if (r < coul_cutoff) {
     qj = moleculeTarget->q[target_id];
     rinv = 1.0/r;
     Ucoul = qi*qj*rinv*KCOUL;
     Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
     r2inv = 1.0/r2;
     fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

     U += Ucoul + Ucoul_shift;
     fx += fcoul*dx;
     fy += fcoul*dy;
     fz += fcoul*dz; 

     r3inv = rinv*r2inv;
     r5inv = r3inv*r2inv;
     rc3inv = 1.0/pow(coul_cutoff,3);
     smooth_factor = (1.0 - r*r2*rc3inv);
     qr3inv = qj*r3inv*smooth_factor;
     qr5inv = -3.0*qj*r5inv*smooth_factor;
     qrc = -3.0*qj*rc3inv*r2inv; 

     Exi = dx * qr3inv;
     Eyi = dy * qr3inv;
     Ezi = dz * qr3inv;

     Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
     Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
     Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

     Exyi = dx*dy*qr5inv + dx*dy*qrc;
     Exzi = dx*dz*qr5inv + dx*dz*qrc;
     Eyzi = dy*dz*qr5inv + dy*dz*qrc;

     Ex += Exi;
     Ey += Eyi;
     Ez += Ezi;

     Exx += Exxi;
     Eyy += Eyyi;
     Ezz += Ezzi;

     Exy += Exyi;
     Exz += Exzi;
     Eyz += Eyzi;
    } 	      
  }
}

// calculation induced dipole interaction on the second neighbors cells
neighborscells = linkedcell->neighbors2_cells[index];
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors2_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);

    if (r < coul_cutoff) {
     qj = moleculeTarget->q[target_id];
     rinv = 1.0/r;
     Ucoul = qi*qj*rinv*KCOUL;
     Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
     r2inv = 1.0/r2;
     fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

     U += Ucoul + Ucoul_shift;
     fx += fcoul*dx;
     fy += fcoul*dy;
     fz += fcoul*dz;

     r3inv = rinv*r2inv;
     r5inv = r3inv*r2inv;
     rc3inv = 1.0/pow(coul_cutoff,3);
     smooth_factor = (1.0 - r*r2*rc3inv);
     qr3inv = qj*r3inv*smooth_factor;
     qr5inv = -3.0*qj*r5inv*smooth_factor;
     qrc = -3.0*qj*rc3inv*r2inv; 

     Exi = dx * qr3inv;
     Eyi = dy * qr3inv;
     Ezi = dz * qr3inv;

     Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
     Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
     Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

     Exyi = dx*dy*qr5inv + dx*dy*qrc;
     Exzi = dx*dz*qr5inv + dx*dz*qrc;
     Eyzi = dy*dz*qr5inv + dy*dz*qrc;

     Ex += Exi;
     Ey += Eyi;
     Ez += Ezi;

     Exx += Exxi;
     Eyy += Eyyi;
     Ezz += Ezzi;

     Exy += Exyi;
     Exz += Exzi;
     Eyz += Eyzi;
    }
  }
}

// induced-dipole contribution
E[0] = Ex;
E[1] = Ey;
E[2] = Ez;

EE[0][0] = Exx; //Exx
EE[0][1] = Exy; //Exy
EE[0][2] = Exz; //Exz

EE[1][0] = Exy; //Eyx
EE[1][1] = Eyy; //Eyy
EE[1][2] = Eyz; //Eyz

EE[2][0] = Exz; //Ezx
EE[2][1] = Eyz; //Ezy
EE[2][2] = Ezz; //Ezz

U_ind = 0.0;
for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    U_ind += -0.5*alpha_tensor[i][j]*E[j]*E[i];
  }  
}

for (int k = 0; k < 3; k++) {
  f_ind[k] = 0.0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      f_ind[k] += 0.5*alpha_tensor[i][j]*(EE[j][k]*E[i] + E[j]*EE[i][k]);
    }  
  }  
}

f[0] = fx + f_ind[0];
f[1] = fy + f_ind[1];
f[2] = fz + f_ind[2];

Up = U + U_ind;
return;
}

/*
 * Compute the lennard-jones, coulomb and induced dipole interactions with anisotropy polarizability
 */

void Force::lennardjones_coulomb_induced_dipole_aniso(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, U;
double dx, dy, dz;
double r2, x2, y2, z2, r;
double r2inv, r6inv;
double fcoul, Ucoul;
double qi, qj, qr3inv, qr5inv, qrc;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double rinv, r3inv, r5inv, r7inv, r9inv, rc3inv;
double U_ind, f_ind[3];
double EE[3][3], E[3], alpha_tensor[3][3];
double alpha_d[3][3], RT[3][3], R[3][3];
double alpha_radial, alpha_axial;
double un[3];
double theta, phi;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double lj1,lj2,lj3,lj4;
double Ulj, flj;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    alpha_d[i][j] = 0.0;
    alpha_tensor[i][j] = 0.0;
  }
}  

alpha_radial = gas->alpha_radial;
alpha_axial = gas->alpha_axial;

alpha_d[0][0] = alpha_radial;
alpha_d[1][1] = alpha_radial;
alpha_d[2][2] = alpha_axial;

// orientation of molecule
un[0] = gas->x[0] - gas->x[2];
un[1] = gas->y[0] - gas->y[2];
un[2] = gas->z[0] - gas->z[2];

double un2;
un2 = gas->d;

un[0] /= un2;
un[1] /= un2;
un[2] /= un2;

theta = acos(un[2]);
phi = atan2(un[1],un[0]);

R[0][0] = cos(phi)*cos(theta);
R[1][0] = sin(phi)*cos(theta);
R[2][0] = -sin(theta);

R[0][1] = -sin(phi);
R[1][1] = cos(phi);
R[2][1] = 0.0;

R[0][2] = cos(phi)*sin(theta);
R[1][2] = sin(phi)*sin(theta);
R[2][2] = cos(theta);

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    RT[i][j] = R[j][i];
  }
}

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 3; k++) {
      for (int m = 0; m < 3; m++) {
        alpha_tensor[i][j] += R[i][k]*alpha_d[k][m]*RT[m][j];
      }
    }
  }
}

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;
Ulj = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  dx = r_probe[0] - moleculeTarget->x[i];
  dy = r_probe[1] - moleculeTarget->y[i];
  dz = r_probe[2] - moleculeTarget->z[i];

  r2 = dx*dx + dy*dy + dz*dz;
  r =  sqrt(r2);

  epsilon = moleculeTarget->eps_central[i];
  sigma = moleculeTarget->sig_central[i];
     
  r2inv = 1.0/r2; 
  r6inv = r2inv*r2inv*r2inv;
  
  lj1 = 4.0*epsilon*pow(sigma,6.0);
  lj2 = lj1*pow(sigma,6.0);
  Ulj = r6inv*(lj2*r6inv - lj1); 
  lj3 = 6.0*lj1;
  lj4 = 12.0*lj2;
  flj = r6inv*(lj4*r6inv - lj3)*r2inv;
  
  U += Ulj;
  fx += flj*dx;
  fy += flj*dy;
  fz += flj*dz;

  qj = moleculeTarget->q[i];
  rinv = 1.0/r;
  Ucoul = qi*qj*rinv*KCOUL;  
  fcoul = Ucoul*r2inv;

  U += Ucoul;
  fx += fcoul*dx;
  fy += fcoul*dy;
  fz += fcoul*dz;

  r3inv = rinv*r2inv;
  r5inv = r3inv*r2inv;
  qr3inv = qj*r3inv;
  qr5inv = qj*r5inv;

  Ex += dx * qr3inv;
  Ey += dy * qr3inv;
  Ez += dz * qr3inv;

  Exx += qr3inv - 3.0*dx*dx*qr5inv;
  Eyy += qr3inv - 3.0*dy*dy*qr5inv;
  Ezz += qr3inv - 3.0*dz*dz*qr5inv;

  Exy += - 3.0 * dx * dy * qr5inv;
  Exz += - 3.0 * dx * dz * qr5inv;
  Eyz += - 3.0 * dy * dz * qr5inv;
}

// induced-dipole contribution
E[0] = Ex;
E[1] = Ey;
E[2] = Ez;

EE[0][0] = Exx; //Exx
EE[1][1] = Eyy; //Eyy
EE[2][2] = Ezz; //Ezz

EE[0][1] = Exy; //Exy
EE[1][0] = EE[0][1]; //Eyx
EE[0][2] = Exz; //Exz
EE[2][0] = EE[0][2]; //Ezx
EE[1][2] = Eyz; //Eyz
EE[2][1] = EE[1][2]; //Ezy

U_ind = 0.0;
for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    U_ind += -0.5*alpha_tensor[i][j]*E[j]*E[i];
  }
}

for (int k = 0; k < 3; k++) {
  f_ind[k] = 0.0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      f_ind[k] += 0.5*alpha_tensor[i][j]*(EE[j][k]*E[i] + E[j]*EE[i][k]);
    }
  }
}

f[0] = fx + f_ind[0];
f[1] = fy + f_ind[1];
f[2] = fz + f_ind[2];

Up = U + U_ind;
return;
}

/*
 * Compute the lennard-jones, coulomb and induced dipole interactions with anisotropy polarizability using linked-cell list
 */

void Force::lennardjones_coulomb_induced_dipole_aniso_LC(GasBuffer *gas, int iatom, vector<double> &f, double &Up) {
double r_probe[3];
double fx, fy,fz, U;
double dx, dy, dz;
double r2, x2, y2, z2, r;
double r2inv, r6inv, rinv;
double qi, qj, qr3inv, qr5inv, qrc;
double Ex, Ey, Ez, Exx, Exy, Exz, Eyy, Eyz, Ezz;
double Exi, Eyi, Ezi, Exxi, Exyi, Exzi, Eyyi, Eyzi, Ezzi;
double r3inv, r5inv, r7inv, r9inv, rc3inv;
double Ucoul, fcoul, Ucoul_shift;
double smooth_factor;
double U_ind, f_ind[3];
double EE[3][3], E[3], alpha_tensor[3][3];
double alpha_d[3][3], RT[3][3], R[3][3];
double theta, phi;
double alpha_axial, alpha_radial, ani_pol;
double un[3];
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double lj1,lj2,lj3,lj4;
double Ulj, flj;
double rc6inv, Ulj_shift;

r_probe[0] = gas->x[iatom];
r_probe[1] = gas->y[iatom];
r_probe[2] = gas->z[iatom];
qi = gas->q[iatom];

int index;
linkedcell->calculateIndex(r_probe,index);

if (index >= linkedcell->Ncells || index < 0) {
  f[0] = 0.0;
  f[1] = 0.0;
  f[2] = 0.0;
  Up = 0.0;
  return;
}

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    alpha_d[i][j] = 0.0;
    alpha_tensor[i][j] = 0.0;
  }
}  

alpha_radial = gas->alpha_radial;
alpha_axial = gas->alpha_axial;

alpha_d[0][0] = alpha_radial;
alpha_d[1][1] = alpha_radial;
alpha_d[2][2] = alpha_axial;

// orientation of molecule
un[0] = gas->x[0] - gas->x[1];
un[1] = gas->y[0] - gas->y[1];
un[2] = gas->z[0] - gas->z[1];

double un2;
un2 = gas->d;

un[0] /= un2;
un[1] /= un2;
un[2] /= un2;

theta = acos(un[2]);
phi = atan2(un[1],un[0]);

R[0][0] = cos(theta)*cos(phi);
R[1][0] = sin(theta)*cos(phi);
R[2][0] = -sin(phi);

R[0][1] = -sin(theta);
R[1][1] = cos(theta);
R[2][1] = 0.0;

R[0][0] = cos(theta)*sin(phi);
R[1][0] = sin(theta)*sin(phi);
R[2][0] = cos(phi);

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    RT[i][j] = R[j][i];
  }
}

for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 3; k++) {
      for (int m = 0; m < 3; m++) {
        alpha_tensor[i][j] += R[i][k]*alpha_d[k][m]*RT[m][j];
      }  
    }
  }
}

U = 0.0;
fx = 0.0;
fy = 0.0;
fz = 0.0;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;
Ulj = 0.0;

int neighborscells, neighbors, target_id;
neighborscells = linkedcell->neighbors1_cells[index];
int cell_index;
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors1_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);

    if (r < lj_cutoff) {
     epsilon = moleculeTarget->eps_central[target_id];
     sigma = moleculeTarget->sig_central[target_id]; 
     r2inv = 1.0/r2;
     r6inv = r2inv*r2inv*r2inv;
     lj1 = 4.0*epsilon*pow(sigma,6.0);
     lj2 = lj1*pow(sigma,6.0);
     Ulj = r6inv*(lj2*r6inv - lj1);
     rc6inv = 1.0/pow(lj_cutoff,6);
     Ulj_shift = rc6inv*(lj2*rc6inv - lj1);
     lj3 = 6.0*lj1;
     lj4 = 12.0*lj2;
     flj = r6inv*(lj4*r6inv - lj3)*r2inv;		
     U += Ulj - Ulj_shift;
     fx += flj*dx;
     fy += flj*dy;
     fz += flj*dz;
    }

    if (r < coul_cutoff) {
      qj = moleculeTarget->q[target_id];
      rinv = 1.0/r;
      Ucoul = qi*qj*rinv*KCOUL;
      Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
      r2inv = 1.0/r2;
      fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

      U += Ucoul + Ucoul_shift;
      fx += fcoul*dx;
      fy += fcoul*dy;
      fz += fcoul*dz; 

      r3inv = rinv*r2inv;
      r5inv = r3inv*r2inv;
      rc3inv = 1.0/pow(coul_cutoff,3);
      smooth_factor = (1.0 - r*r2*rc3inv);
      qr3inv = qj*r3inv*smooth_factor;
      qr5inv = -3.0*qj*r5inv*smooth_factor;
      qrc = -3.0*qj*rc3inv*r2inv; 

      Exi = dx * qr3inv;
      Eyi = dy * qr3inv;
      Ezi = dz * qr3inv;

      Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
      Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
      Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

      Exyi = dx*dy*qr5inv + dx*dy*qrc;
      Exzi = dx*dz*qr5inv + dx*dz*qrc;
      Eyzi = dy*dz*qr5inv + dy*dz*qrc;

      Ex += Exi;
      Ey += Eyi;
      Ez += Ezi;

      Exx += Exxi;
      Eyy += Eyyi;
      Ezz += Ezzi;

      Exy += Exyi;
      Exz += Exzi;
      Eyz += Eyzi;
    } 	      
  }
}

// calculation induced dipole interaction on the second neighbors cells
neighborscells = linkedcell->neighbors2_cells[index];
for (int i = 0; i < neighborscells; i++) {
  cell_index = linkedcell->neighbors2_cells_ids[index][i];
  neighbors = linkedcell->atoms_inside_cell[cell_index];
  #pragma omp simd
  for (int j = 0; j < neighbors; j++) {
    target_id = linkedcell->atoms_ids[cell_index][j];
    dx = r_probe[0] - moleculeTarget->x[target_id];
    dy = r_probe[1] - moleculeTarget->y[target_id];
    dz = r_probe[2] - moleculeTarget->z[target_id];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);

    if (r < coul_cutoff) {
     qj = moleculeTarget->q[target_id];
     rinv = 1.0/r;
     Ucoul = qi*qj*rinv*KCOUL;
     Ucoul_shift = -1.5*qi*qj*KCOUL/coul_cutoff + 0.5*qi*qj*KCOUL*r2/pow(coul_cutoff,3); 
     r2inv = 1.0/r2;
     fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    

     U += Ucoul + Ucoul_shift;
     fx += fcoul*dx;
     fy += fcoul*dy;
     fz += fcoul*dz;

     r3inv = rinv*r2inv;
     r5inv = r3inv*r2inv;
     rc3inv = 1.0/pow(coul_cutoff,3);
     smooth_factor = (1.0 - r*r2*rc3inv);
     qr3inv = qj*r3inv*smooth_factor;
     qr5inv = -3.0*qj*r5inv*smooth_factor;
     qrc = -3.0*qj*rc3inv*r2inv; 

     Exi = dx * qr3inv;
     Eyi = dy * qr3inv;
     Ezi = dz * qr3inv;

     Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
     Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
     Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

     Exyi = dx*dy*qr5inv + dx*dy*qrc;
     Exzi = dx*dz*qr5inv + dx*dz*qrc;
     Eyzi = dy*dz*qr5inv + dy*dz*qrc;

     Ex += Exi;
     Ey += Eyi;
     Ez += Ezi;

     Exx += Exxi;
     Eyy += Eyyi;
     Ezz += Ezzi;

     Exy += Exyi;
     Exz += Exzi;
     Eyz += Eyzi;
    }
  }
}

// induced-dipole contribution
E[0] = Ex;
E[1] = Ey;
E[2] = Ez;

EE[0][0] = Exx; //Exx
EE[0][1] = Exy; //Exy
EE[0][2] = Exz; //Exz

EE[1][0] = Exy; //Eyx
EE[1][1] = Eyy; //Eyy
EE[1][2] = Eyz; //Eyz

EE[2][0] = Exz; //Ezx
EE[2][1] = Eyz; //Ezy
EE[2][2] = Ezz; //Ezz

U_ind = 0.0;
for (int i = 0; i < 3; i++) {
  for (int j = 0; j < 3; j++) {
    U_ind += -0.5*alpha_tensor[i][j]*E[j]*E[i];
  }  
}

for (int k = 0; k < 3; k++) {
  f_ind[k] = 0.0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      f_ind[k] += 0.5*alpha_tensor[i][j]*(EE[j][k]*E[i] + E[j]*EE[i][k]);
    }  
  }  
}

f[0] = fx + f_ind[0];
f[1] = fy + f_ind[1];
f[2] = fz + f_ind[2];

Up = U + U_ind;
return;
}

// average weigthing force over 3 directions

// Nitrogen pseudo-particle  
void Force::average_weighting_force_N2(GasBuffer *gas, vector<double> &f, double &Up) {
double r_probe[3], r_target[3];
double r_N[6][3];
double fx, fy,fz, Ulj, flj, U;
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double r2,r;
double rinv,r2inv,r3inv,r5inv,r6inv;
double lj1,lj2,lj3,lj4;
double qC, qN, q, rd;
double Ucoul, fcoul;
int index;

r_probe[0] = gas->x[0];
r_probe[1] = gas->y[0];
r_probe[2] = gas->z[0];
qC = gas->q[0];
qN = -0.5*qC;
rd = 0.5*gas->d;

// 1 axis X
r_N[0][0] = r_probe[0] + rd;
r_N[0][1] = r_probe[1];
r_N[0][2] = r_probe[2];
// 2 axis X
r_N[1][0] = r_probe[0] - rd;
r_N[1][1] = r_probe[1];
r_N[1][2] = r_probe[2];
// 1 axis Y
r_N[2][0] = r_probe[0];
r_N[2][1] = r_probe[1] + rd;
r_N[2][2] = r_probe[2];
// 2 axis Y
r_N[3][0] = r_probe[0];
r_N[3][1] = r_probe[1] - rd;
r_N[3][2] = r_probe[2];
// 1 axis Z
r_N[4][0] = r_probe[0];
r_N[4][1] = r_probe[1];
r_N[4][2] = r_probe[2] + rd;
// 2 axis Z
r_N[5][0] = r_probe[0];
r_N[5][1] = r_probe[1];
r_N[5][2] = r_probe[2] - rd;

double Ulj_N[6], flj_x[6], flj_y[6], flj_z[6];
double Ucoul_N[6], fcoul_Nx[6], fcoul_Ny[6], fcoul_Nz[6];
for (int i = 0; i < 6; i++) {
  Ulj_N[i] = 0.0;
  flj_x[i] = 0.0;
  flj_y[i] = 0.0;
  flj_z[i] = 0.0;
  Ucoul_N[i] = 0.0;
  fcoul_Nx[i] = 0.0;
  fcoul_Ny[i] = 0.0;
  fcoul_Nz[i] = 0.0;
}

double qr3inv, qr5inv;
double Ex, Ey, Ez, Exx, Eyy, Ezz, Exy, Exz, Eyz, Ucoul_C, fcoul_Cx, fcoul_Cy, fcoul_Cz;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;
Ucoul_C = 0.0;
fcoul_Cx = 0.0;
fcoul_Cy = 0.0;
fcoul_Cz = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  r_target[0] = moleculeTarget->x[i];
  r_target[1] = moleculeTarget->y[i];
  r_target[2] = moleculeTarget->z[i];

  epsilon = moleculeTarget->eps[i];
  sigma = moleculeTarget->sig[i];
  q = moleculeTarget->q[i];

  // nitrogen calculations
  for (int k = 0; k < 6; k++) {
    dx = r_N[k][0] - r_target[0];
    dy = r_N[k][1] - r_target[1];
    dz = r_N[k][2] - r_target[2];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);
        
 	  r2inv = 1.0/r2;     
    r6inv = r2inv*r2inv*r2inv;
    lj1 = 4.0*epsilon*pow(sigma,6.0);
    lj2 = lj1*pow(sigma,6.0);
    Ulj = r6inv*(lj2*r6inv - lj1);

    lj3 = 6.0*lj1;
    lj4 = 12.0*lj2;
    flj = r6inv*(lj4*r6inv - lj3)*r2inv;

	  Ulj_N[k] += Ulj;
    flj_x[k] += flj*dx;
    flj_y[k] += flj*dy;
    flj_z[k] += flj*dz;
    
	  rinv = 1.0/r;
	  Ucoul = qN*q*rinv*KCOUL;
    r2inv = 1.0/r2;
    fcoul = Ucoul*r2inv;

    Ucoul_N[k] += Ucoul;
	  fcoul_Nx[k] += fcoul*dx;
	  fcoul_Ny[k] += fcoul*dy;
    fcoul_Nz[k] += fcoul*dz;     
  }

  // central particle calculations
  dx = r_probe[0] - r_target[0];
  dy = r_probe[1] - r_target[1];
  dz = r_probe[2] - r_target[2];
  r2 = dx*dx + dy*dy + dz*dz;
  r =  sqrt(r2);
  
	rinv = 1.0/r;
	Ucoul = qC*q*rinv*KCOUL;
	r2inv = 1.0/r2;
  fcoul = Ucoul*r2inv;
  
  Ucoul_C += Ucoul;
	fcoul_Cx += fcoul*dx;
	fcoul_Cy += fcoul*dy;
  fcoul_Cz += fcoul*dz; 
  
  r3inv = rinv*r2inv;
  r5inv = r3inv*r2inv;
  qr3inv = q*r3inv;
  qr5inv = q*r5inv;

  Ex += dx * qr3inv;
  Ey += dy * qr3inv;
  Ez += dz * qr3inv;

  Exx += qr3inv - 3.0*dx*dx*qr5inv;
  Eyy += qr3inv - 3.0*dy*dy*qr5inv;
  Ezz += qr3inv - 3.0*dz*dz*qr5inv;

  Exy += - 3.0 * dx * dy * qr5inv;
  Exz += - 3.0 * dx * dz * qr5inv;
  Eyz += - 3.0 * dy * dz * qr5inv;
}

double Uind, find[3];
Uind = -0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
find[0] = alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
find[1] = alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
find[2] = alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

double Umol[3], fmolX[3], fmolY[3], fmolZ[3];
Umol[0] = Ulj_N[0] + Ulj_N[1] + Ucoul_N[0] + Ucoul_N[1] + Uind + Ucoul_C;
Umol[1] = Ulj_N[2] + Ulj_N[3] + Ucoul_N[2] + Ucoul_N[3] + Uind + Ucoul_C;
Umol[2] = Ulj_N[4] + Ulj_N[5] + Ucoul_N[4] + Ucoul_N[5] + Uind + Ucoul_C;

double Umin;

Umin = min(Umol[0],min(Umol[1],Umol[2]));

fmolX[0] = flj_x[0] + flj_x[1] + fcoul_Nx[0] + fcoul_Nx[1] + find[0] + fcoul_Cx;
fmolY[0] = flj_y[0] + flj_y[1] + fcoul_Ny[0] + fcoul_Ny[1] + find[1] + fcoul_Cy;
fmolZ[0] = flj_z[0] + flj_z[1] + fcoul_Nz[0] + fcoul_Nz[1] + find[2] + fcoul_Cz;

fmolX[1] = flj_x[2] + flj_x[3] + fcoul_Nx[2] + fcoul_Nx[3] + find[0] + fcoul_Cx;
fmolY[1] = flj_y[2] + flj_y[3] + fcoul_Ny[2] + fcoul_Ny[3] + find[1] + fcoul_Cy;
fmolZ[1] = flj_z[2] + flj_z[3] + fcoul_Nz[2] + fcoul_Nz[3] + find[2] + fcoul_Cy;

fmolX[2] = flj_x[4] + flj_x[5] + fcoul_Nx[4] + fcoul_Nx[5] + find[0] + fcoul_Cx;
fmolY[2] = flj_y[4] + flj_y[5] + fcoul_Ny[4] + fcoul_Ny[5] + find[1] + fcoul_Cy;
fmolZ[2] = flj_z[4] + flj_z[5] + fcoul_Nz[4] + fcoul_Nz[5] + find[2] + fcoul_Cz;

double dU, Z, kBT, T;
dU =0.0;
Z = 0.0;
T = 500.0;
kBT = BOLTZMANN_K * T * J_TO_eV * eV_TO_KCAL_MOL;

for (int i = 0; i < 3; i++) {
  dU = Umol[i] - Umin;
  Z += exp(-dU/kBT);
}

Up = 0.0;
f[0] = 0.0;
f[1] = 0.0;
f[2] = 0.0;

double w;

for (int i = 0; i < 3; i++) {
  dU = Umol[i] - Umin;
  w = exp(-dU/kBT)/Z;
  Up += w * Umol[i];
  f[0] += w * fmolX[i];
  f[1] += w * fmolY[i];
  f[2] += w * fmolZ[i];
}

return;
}

// average weighting force using linked-cell list
void Force::average_weighting_force_LC_N2(GasBuffer *gas, vector<double> &f, double &Up) {
double r_probe[3], r_target[3];
double r_N[6][3];
double fx, fy, fz, Ulj, flj, U;
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double r2, r;
double rinv,r2inv,r3inv,r5inv,r6inv;
double lj1,lj2,lj3,lj4;
double qC, qN, q, rd;
double Ucoul, fcoul;
int index;
bool inside;

// central particle
r_probe[0] = gas->x[0];
r_probe[1] = gas->y[0];
r_probe[2] = gas->z[0];
qC = gas->q[0];
qN = -0.5*qC;

linkedcell->calculateIndex(r_probe,index);

// central particle
if (index >= linkedcell->Ncells && index < 0) {
  inside = false;
} else inside = true;

double qr3inv, qr5inv;
double Ex, Ey, Ez, Exx, Eyy, Ezz, Exy, Exz, Eyz, Ucoul_C, fcoul_Cx, fcoul_Cy, fcoul_Cz;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;
Ucoul_C = 0.0;
fcoul_Cx = 0.0;
fcoul_Cy = 0.0;
fcoul_Cz = 0.0;

double s1, s2, Exi, Eyi, Ezi, Exxi, Eyyi, Ezzi, Exyi, Exzi, Eyzi;
int neighborscells, neighbors, target_id;
int cell_index;
double Ucoul_shift, rc3inv, smooth_factor, qrc;

if (inside == true) {  
  neighborscells = linkedcell->neighbors1_cells[index];
  for (int i = 0; i < neighborscells; i++) {;
    cell_index = linkedcell->neighbors1_cells_ids[index][i];
    neighbors = linkedcell->atoms_inside_cell[cell_index];
    #pragma omp simd
    for (int j = 0; j < neighbors; j++) {
      target_id = linkedcell->atoms_ids[cell_index][j];
      r_target[0] = moleculeTarget->x[target_id];
      r_target[1] = moleculeTarget->y[target_id];
      r_target[2] = moleculeTarget->z[target_id];
 
      dx = r_probe[0] - r_target[0];
      dy = r_probe[1] - r_target[1];
      dz = r_probe[2] - r_target[2];
      r2 = dx*dx + dy*dy + dz*dz;
      r = sqrt(r2);

      if (r < coul_cutoff) {
        q = moleculeTarget->q[target_id];
	      rinv = 1.0/r;
	      Ucoul = qC*q*rinv*KCOUL;
        Ucoul_shift = -1.5*qC*q*KCOUL/coul_cutoff + 0.5*qC*q*KCOUL*r2/pow(coul_cutoff,3);
	      r2inv = 1.0/r2;
        fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));

        Ucoul_C += Ucoul + Ucoul_shift;
	      fcoul_Cx += fcoul*dx;
	      fcoul_Cy += fcoul*dy;
        fcoul_Cz += fcoul*dz; 

        r3inv = rinv*r2inv;
        r5inv = r3inv*r2inv;
        rc3inv = 1.0/pow(coul_cutoff,3);
        smooth_factor = (1.0 - r*r2*rc3inv);
        qr3inv = q*r3inv*smooth_factor;
        qr5inv = -3.0*q*r5inv*smooth_factor;
        qrc = -3.0*q*rc3inv*r2inv; 

        Exi = dx * qr3inv;
        Eyi = dy * qr3inv;
        Ezi = dz * qr3inv;

        Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
        Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
        Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

        Exyi = dx*dy*qr5inv + dx*dy*qrc;
        Exzi = dx*dz*qr5inv + dx*dz*qrc;
        Eyzi = dy*dz*qr5inv + dy*dz*qrc;                 
  
        Ex += Exi;
        Ey += Eyi;
        Ez += Ezi;

        Exx += Exxi;
        Eyy += Eyyi;
        Ezz += Ezzi;

        Exy += Exyi;
        Exz += Exzi;
        Eyz += Eyzi;  
      }
    }
  }
  
  neighborscells = linkedcell->neighbors2_cells[index];  
  for (int i = 0; i < neighborscells; i++) {
    cell_index = linkedcell->neighbors2_cells_ids[index][i];
    neighbors = linkedcell->atoms_inside_cell[cell_index];
    #pragma omp simd
    for (int j = 0; j < neighbors; j++) {
      target_id = linkedcell->atoms_ids[cell_index][j];
      r_target[0] = moleculeTarget->x[target_id];
      r_target[1] = moleculeTarget->y[target_id];
      r_target[2] = moleculeTarget->z[target_id];

      dx = r_probe[0] - r_target[0];
      dy = r_probe[1] - r_target[1];
      dz = r_probe[2] - r_target[2];
      r2 = dx*dx + dy*dy + dz*dz;
      r = sqrt(r2);

      if (r < coul_cutoff) {
        q = moleculeTarget->q[target_id];
	      rinv = 1.0/r;
	      Ucoul = qC*q*rinv*KCOUL;
        Ucoul_shift = -1.5*qC*q*KCOUL/coul_cutoff + 0.5*qC*q*KCOUL*r2/pow(coul_cutoff,3); 
        r2inv = 1.0/r2;
        fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    
	    
        Ucoul_C += Ucoul;
	      fcoul_Cx += fcoul*dx;
	      fcoul_Cy += fcoul*dy;
        fcoul_Cz += fcoul*dz; 

        r3inv = rinv*r2inv;
        r5inv = r3inv*r2inv;
        rc3inv = 1.0/pow(coul_cutoff,3);
        smooth_factor = (1.0 - r*r2*rc3inv);
        qr3inv = q*r3inv*smooth_factor;
        qr5inv = -3.0*q*r5inv*smooth_factor;
        qrc = -3.0*q*rc3inv*r2inv; 

        Exi = dx * qr3inv;
        Eyi = dy * qr3inv;
        Ezi = dz * qr3inv;

        Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
        Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
        Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

        Exyi = dx*dy*qr5inv + dx*dy*qrc;
        Exzi = dx*dz*qr5inv + dx*dz*qrc;
        Eyzi = dy*dz*qr5inv + dy*dz*qrc;
  
        Ex += Exi;
        Ey += Eyi;
        Ez += Ezi;

        Exx += Exxi;
        Eyy += Eyyi;
        Ezz += Ezzi;

        Exy += Exyi;
        Exz += Exzi;
        Eyz += Eyzi;
      }  
    }
  }    
}

// nitrogen particles
rd = 0.5*gas->d;

// 1 axis X
r_N[0][0] = r_probe[0] + rd;
r_N[0][1] = r_probe[1];
r_N[0][2] = r_probe[2];
// 2 axis X
r_N[1][0] = r_probe[0] - rd;
r_N[1][1] = r_probe[1];
r_N[1][2] = r_probe[2];
// 1 axis Y
r_N[2][0] = r_probe[0];
r_N[2][1] = r_probe[1] + rd;
r_N[2][2] = r_probe[2];
// 2 axis Y
r_N[3][0] = r_probe[0];
r_N[3][1] = r_probe[1] - rd;
r_N[3][2] = r_probe[2];
// 1 axis Z
r_N[4][0] = r_probe[0];
r_N[4][1] = r_probe[1];
r_N[4][2] = r_probe[2] + rd;
// 2 axis Z
r_N[5][0] = r_probe[0];
r_N[5][1] = r_probe[1];
r_N[5][2] = r_probe[2] - rd;

double Ulj_N[6], flj_x[6], flj_y[6], flj_z[6];
double Ucoul_N[6], fcoul_Nx[6], fcoul_Ny[6], fcoul_Nz[6];
double pos_N[3], rc6inv, Ulj_cut;

for (int k = 0; k < 6; k++) {
  pos_N[0] = r_N[k][0];
  pos_N[1] = r_N[k][1];
  pos_N[2] = r_N[k][2];
  
  linkedcell->calculateIndex(pos_N,index);

  // central particle
  if (index >= linkedcell->Ncells && index < 0) {    
    inside = false;
  } else inside = true;
  
  Ulj_N[k] = 0.0;
  flj_x[k] = 0.0;
  flj_y[k] = 0.0;
  flj_z[k] = 0.0;
  Ucoul_N[k] = 0.0;
  fcoul_Nx[k] = 0.0;
  fcoul_Ny[k] = 0.0;
  fcoul_Nz[k] = 0.0;

  if (inside == true) {
    neighborscells = linkedcell->neighbors1_cells[index];
    for (int i = 0; i < neighborscells; i++) {
      cell_index = linkedcell->neighbors1_cells_ids[index][i];
      neighbors = linkedcell->atoms_inside_cell[cell_index];
      #pragma omp simd
      for (int j = 0; j < neighbors; j++) {
        target_id = linkedcell->atoms_ids[cell_index][j];
        r_target[0] = moleculeTarget->x[target_id];
        r_target[1] = moleculeTarget->y[target_id];
        r_target[2] = moleculeTarget->z[target_id];
    
        dx = pos_N[0] - r_target[0];
        dy = pos_N[1] - r_target[1];
        dz = pos_N[2] - r_target[2];
        r2 = dx*dx + dy*dy + dz*dz;
        r =  sqrt(r2);

        if (r < lj_cutoff) {
          epsilon = moleculeTarget->eps[target_id];
          sigma = moleculeTarget->sig[target_id]; 
          r2inv = 1.0/r2;
          r6inv = r2inv*r2inv*r2inv;
          lj1 = 4.0*epsilon*pow(sigma,6.0);
          lj2 = lj1*pow(sigma,6.0);
          Ulj = r6inv*(lj2*r6inv - lj1);
          rc6inv = 1.0/pow(lj_cutoff,6);
          Ulj_cut = rc6inv*(lj2*rc6inv - lj1);
          lj3 = 6.0*lj1;
          lj4 = 12.0*lj2;
          flj = r6inv*(lj4*r6inv - lj3)*r2inv;
	      
          Ulj_N[k] += Ulj - Ulj_cut;
          flj_x[k] += flj*dx;
          flj_y[k] += flj*dy;
          flj_z[k] += flj*dz;
        }  

        if (r < coul_cutoff) {
          q = moleculeTarget->q[target_id];
          rinv = 1.0/r;
          Ucoul = qN*q*rinv*KCOUL;
          Ucoul_shift = -1.5*qN*q*KCOUL/coul_cutoff + 0.5*qN*q*KCOUL*r2/pow(coul_cutoff,3); 
          r2inv = 1.0/r2;
          fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));
          
          Ucoul_N[k] += Ucoul + Ucoul_shift;
          fcoul_Nx[k] += fcoul*dx;
          fcoul_Ny[k] += fcoul*dy;
          fcoul_Nz[k] += fcoul*dz; 
        }
      }
    }
    
    neighborscells = linkedcell->neighbors2_cells[index];
    for (int i = 0; i < neighborscells; i++) {
      cell_index = linkedcell->neighbors2_cells_ids[index][i];
      neighbors = linkedcell->atoms_inside_cell[cell_index];
      #pragma omp simd
      for (int j = 0; j < neighbors; j++) {
        target_id = linkedcell->atoms_ids[cell_index][j];
        r_target[0] = moleculeTarget->x[target_id];
        r_target[1] = moleculeTarget->y[target_id];
        r_target[2] = moleculeTarget->z[target_id];

        dx = pos_N[0] - r_target[0];
        dy = pos_N[1] - r_target[1];
        dz = pos_N[2] - r_target[2];
        r2 = dx*dx + dy*dy + dz*dz;
        r =  sqrt(r2);  
         
        if (r < coul_cutoff) {
          q = moleculeTarget->q[target_id];
	        rinv = 1.0/r;
	        Ucoul = qN*q*rinv*KCOUL;
          Ucoul_shift = -1.5*qN*q*KCOUL/coul_cutoff + 0.5*qN*q*KCOUL*r2/pow(coul_cutoff,3); 
	        r2inv = 1.0/r2;
          fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));
                   
          Ucoul_N[k] += Ucoul + Ucoul_shift;
	        fcoul_Nx[k] += fcoul*dx;
	        fcoul_Ny[k] += fcoul*dy;
          fcoul_Nz[k] += fcoul*dz; 
        } 
      }  
    }  
  }
}

double Uind, find[3];
Uind = -0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
find[0] = alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
find[1] = alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
find[2] = alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

double Umol[3], fmolX[3],fmolY[3],fmolZ[3];
Umol[0] = Ulj_N[0] + Ulj_N[1] + Ucoul_N[0] + Ucoul_N[1] + Uind + Ucoul_C;
Umol[1] = Ulj_N[2] + Ulj_N[3] + Ucoul_N[2] + Ucoul_N[3] + Uind + Ucoul_C;
Umol[2] = Ulj_N[4] + Ulj_N[5] + Ucoul_N[4] + Ucoul_N[5] + Uind + Ucoul_C;

double Umin;

Umin = min(Umol[0],min(Umol[1],Umol[2]));

fmolX[0] = flj_x[0] + flj_x[1] + fcoul_Nx[0] + fcoul_Nx[1] + find[0] + fcoul_Cx;
fmolY[0] = flj_y[0] + flj_y[1] + fcoul_Ny[0] + fcoul_Ny[1] + find[1] + fcoul_Cy;
fmolZ[0] = flj_z[0] + flj_z[1] + fcoul_Nz[0] + fcoul_Nz[1] + find[2] + fcoul_Cz;

fmolX[1] = flj_x[2] + flj_x[3] + fcoul_Nx[2] + fcoul_Nx[3] + find[0] + fcoul_Cx;
fmolY[1] = flj_y[2] + flj_y[3] + fcoul_Ny[2] + fcoul_Ny[3] + find[1] + fcoul_Cy;
fmolZ[1] = flj_z[2] + flj_z[3] + fcoul_Nz[2] + fcoul_Nz[3] + find[2] + fcoul_Cy;

fmolX[2] = flj_x[4] + flj_x[5] + fcoul_Nx[4] + fcoul_Nx[5] + find[0] + fcoul_Cx;
fmolY[2] = flj_y[4] + flj_y[5] + fcoul_Ny[4] + fcoul_Ny[5] + find[1] + fcoul_Cy;
fmolZ[2] = flj_z[4] + flj_z[5] + fcoul_Nz[4] + fcoul_Nz[5] + find[2] + fcoul_Cz;

double dU, Z, kBT, T;
Z = 0.0;
T = 500.0;
kBT = BOLTZMANN_K * T * J_TO_eV * eV_TO_KCAL_MOL;

for (int i = 0; i < 3; i++) {
  dU = Umol[i] - Umin;
  Z += exp(-dU/kBT);
}

Up = 0.0;
f[0] = 0.0;
f[1] = 0.0;
f[2] = 0.0;

double w;
for (int i = 0; i < 3; i++) {
  dU = Umol[i] - Umin;
  w = exp(-dU/kBT)/Z;
  Up += w * Umol[i];
  f[0] += w * fmolX[i];
  f[1] += w * fmolY[i];
  f[2] += w * fmolZ[i];
}

return;
}

// average weighting force for CO2

void Force::average_weighting_force_CO2(GasBuffer *gas, vector<double> &f, double &Up) {
double r_probe[3], r_target[3];
double r_O[6][3];
double fx, fy,fz, Ulj, flj, U;
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target, epsilon_central;
double sigma_probe, sigma_target, sigma_central;
double r2,r;
double rinv,r2inv,r3inv,r5inv,r6inv;
double lj1,lj2,lj3,lj4;
double qC, qO, q, rd;
double Ucoul, fcoul;
int index;

r_probe[0] = gas->x[0];
r_probe[1] = gas->y[0];
r_probe[2] = gas->z[0];
qC = gas->q[1];
qO = -0.5*qC;
rd = 0.5*gas->d;

// 1 axis X
r_O[0][0] = r_probe[0] + rd;
r_O[0][1] = r_probe[1];
r_O[0][2] = r_probe[2];
// 2 axis X
r_O[1][0] = r_probe[0] - rd;
r_O[1][1] = r_probe[1];
r_O[1][2] = r_probe[2];
// 1 axis Y
r_O[2][0] = r_probe[0];
r_O[2][1] = r_probe[1] + rd;
r_O[2][2] = r_probe[2];
// 2 axis Y
r_O[3][0] = r_probe[0];
r_O[3][1] = r_probe[1] - rd;
r_O[3][2] = r_probe[2];
// 1 axis Z
r_O[4][0] = r_probe[0];
r_O[4][1] = r_probe[1];
r_O[4][2] = r_probe[2] + rd;
// 2 axis Z
r_O[5][0] = r_probe[0];
r_O[5][1] = r_probe[1];
r_O[5][2] = r_probe[2] - rd;

double Ulj_O[6], flj_x[6], flj_y[6], flj_z[6];
double Ucoul_O[6], fcoul_Ox[6], fcoul_Oy[6], fcoul_Oz[6];

for (int i = 0; i < 6; i++) {
  Ulj_O[i] = 0.0;
  flj_x[i] = 0.0;
  flj_y[i] = 0.0;
  flj_z[i] = 0.0;
  Ucoul_O[i] = 0.0;
  fcoul_Ox[i] = 0.0;
  fcoul_Oy[i] = 0.0;
  fcoul_Oz[i] = 0.0;
}

double Ulj_C, flj_Cx, flj_Cy, flj_Cz;
Ulj_C = 0.0;
flj_Cx = 0.0;
flj_Cy = 0.0;
flj_Cz = 0.0;

double qr3inv, qr5inv;
double Ex, Ey, Ez, Exx, Eyy, Ezz, Exy, Exz, Eyz, Ucoul_C, fcoul_Cx, fcoul_Cy, fcoul_Cz;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;
Ucoul_C = 0.0;
fcoul_Cx = 0.0;
fcoul_Cy = 0.0;
fcoul_Cz = 0.0;

#pragma omp simd
for (int i = 0; i < moleculeTarget->natoms; i++) {
  r_target[0] = moleculeTarget->x[i];
  r_target[1] = moleculeTarget->y[i];
  r_target[2] = moleculeTarget->z[i];

  epsilon = moleculeTarget->eps[i];
  sigma = moleculeTarget->sig[i];
  q = moleculeTarget->q[i];

  // oxygen calculations
  for (int k = 0; k < 6; k++) {
    dx = r_O[k][0] - r_target[0];
    dy = r_O[k][1] - r_target[1];
    dz = r_O[k][2] - r_target[2];
    r2 = dx*dx + dy*dy + dz*dz;
    r = sqrt(r2);
    
 	  r2inv = 1.0/r2;     
    r6inv = r2inv*r2inv*r2inv;
    lj1 = 4.0*epsilon*pow(sigma,6.0);
    lj2 = lj1*pow(sigma,6.0);
    Ulj = r6inv*(lj2*r6inv - lj1);

    lj3 = 6.0*lj1;
    lj4 = 12.0*lj2;
    flj = r6inv*(lj4*r6inv - lj3)*r2inv;

	  Ulj_O[k] += Ulj;
    flj_x[k] += flj*dx;
    flj_y[k] += flj*dy;
    flj_z[k] += flj*dz;
    
	  rinv = 1.0/r;
	  Ucoul = qO*q*rinv*KCOUL;
    r2inv = 1.0/r2;
    fcoul = Ucoul*r2inv;

    Ucoul_O[k] += Ucoul;
	  fcoul_Ox[k] += fcoul*dx;
	  fcoul_Oy[k] += fcoul*dy;
    fcoul_Oz[k] += fcoul*dz; 
  }

  // central particle calculations
  dx = r_probe[0] - r_target[0];
  dy = r_probe[1] - r_target[1];
  dz = r_probe[2] - r_target[2];
  r2 = dx*dx + dy*dy + dz*dz;
  r =  sqrt(r2);
  
  r2inv = 1.0/r2;     
  r6inv = r2inv*r2inv*r2inv;
  epsilon_central = moleculeTarget->eps_central[i];
  sigma_central = moleculeTarget->sig_central[i];
  lj1 = 4.0*epsilon_central*pow(sigma_central,6);
  lj2 = lj1*pow(sigma_central,6);
  Ulj = r6inv*(lj2*r6inv - lj1);
  lj3 = 6.0*lj1;
  lj4 = 12.0*lj2;
  flj = r6inv*(lj4*r6inv - lj3)*r2inv;

  Ulj_C += Ulj;
  flj_Cx += flj*dx;
  flj_Cy += flj*dy;
  flj_Cz += flj*dz;
  
	rinv = 1.0/r;
	Ucoul = qC*q*rinv*KCOUL;
	r2inv = 1.0/r2;
  fcoul = Ucoul*r2inv;
  
  Ucoul_C += Ucoul;
	fcoul_Cx += fcoul*dx;
	fcoul_Cy += fcoul*dy;
  fcoul_Cz += fcoul*dz; 
  
  r3inv = rinv*r2inv;
  r5inv = r3inv*r2inv;
  qr3inv = q*r3inv;
  qr5inv = q*r5inv;

  Ex += dx * qr3inv;
  Ey += dy * qr3inv;
  Ez += dz * qr3inv;

  Exx += qr3inv - 3.0*dx*dx*qr5inv;
  Eyy += qr3inv - 3.0*dy*dy*qr5inv;
  Ezz += qr3inv - 3.0*dz*dz*qr5inv;

  Exy += - 3.0 * dx * dy * qr5inv;
  Exz += - 3.0 * dx * dz * qr5inv;
  Eyz += - 3.0 * dy * dz * qr5inv;  
}

double Uind, find[3];
Uind = -0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
find[0] = alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
find[1] = alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
find[2] = alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

double Umol[3], fmolX[3], fmolY[3], fmolZ[3];
Umol[0] = Ulj_O[0] + Ulj_O[1] + Ucoul_O[0] + Ucoul_O[1] + Uind + Ucoul_C + Ulj_C;
Umol[1] = Ulj_O[2] + Ulj_O[3] + Ucoul_O[2] + Ucoul_O[3] + Uind + Ucoul_C + Ulj_C;
Umol[2] = Ulj_O[4] + Ulj_O[5] + Ucoul_O[4] + Ucoul_O[5] + Uind + Ucoul_C + Ulj_C;

double Umin;

Umin = min(Umol[0],min(Umol[1],Umol[2]));

fmolX[0] = flj_x[0] + flj_x[1] + fcoul_Ox[0] + fcoul_Ox[1] + find[0] + fcoul_Cx + flj_Cx;
fmolY[0] = flj_y[0] + flj_y[1] + fcoul_Oy[0] + fcoul_Oy[1] + find[1] + fcoul_Cy + flj_Cy;
fmolZ[0] = flj_z[0] + flj_z[1] + fcoul_Oz[0] + fcoul_Oz[1] + find[2] + fcoul_Cz + flj_Cz;

fmolX[1] = flj_x[2] + flj_x[3] + fcoul_Ox[2] + fcoul_Ox[3] + find[0] + fcoul_Cx + flj_Cx;
fmolY[1] = flj_y[2] + flj_y[3] + fcoul_Oy[2] + fcoul_Oy[3] + find[1] + fcoul_Cy + flj_Cy;
fmolZ[1] = flj_z[2] + flj_z[3] + fcoul_Oz[2] + fcoul_Oz[3] + find[2] + fcoul_Cy + flj_Cz;

fmolX[2] = flj_x[4] + flj_x[5] + fcoul_Ox[4] + fcoul_Ox[5] + find[0] + fcoul_Cx + flj_Cx;
fmolY[2] = flj_y[4] + flj_y[5] + fcoul_Oy[4] + fcoul_Oy[5] + find[1] + fcoul_Cy + flj_Cy;
fmolZ[2] = flj_z[4] + flj_z[5] + fcoul_Oz[4] + fcoul_Oz[5] + find[2] + fcoul_Cz + flj_Cz;

double dU, Z, kBT, T;
dU =0.0;
Z = 0.0;
T = 500.0;
kBT = BOLTZMANN_K * T * J_TO_eV * eV_TO_KCAL_MOL;

for (int i = 0; i < 3; i++) {
  dU = Umol[i] - Umin;
  Z += exp(-dU/kBT);
}

Up = 0.0;
f[0] = 0.0;
f[1] = 0.0;
f[2] = 0.0;

double w;

for (int i = 0; i < 3; i++) {
  dU = Umol[i] - Umin;
  w = exp(-dU/kBT)/Z;
  Up += w * Umol[i];
  f[0] += w * fmolX[i];
  f[1] += w * fmolY[i];
  f[2] += w * fmolZ[i];
}

return;
}

// average weighting force using linked-cell list
void Force::average_weighting_force_LC_CO2(GasBuffer *gas, vector<double> &f, double &Up) {
double r_probe[3], r_target[3];
double r_O[6][3];
double fx, fy, fz, Ulj, flj, U;
double dx, dy, dz;
double epsilon, sigma;
double epsilon_probe, epsilon_target;
double sigma_probe, sigma_target;
double r2, r;
double rinv,r2inv,r3inv,r5inv,r6inv;
double lj1,lj2,lj3,lj4;
double qC, qO, q, d;
double Ucoul, fcoul, flj_C, Ulj_C;
int index;
bool inside;

// central particle
r_probe[0] = gas->x[0];
r_probe[1] = gas->y[0];
r_probe[2] = gas->z[0];
qC = gas->q[0];
qO = -0.5*qC;

linkedcell->calculateIndex(r_probe,index);

// central particle
if (index >= linkedcell->Ncells && index < 0) {
  inside = false;
} else inside = true;

double qr3inv, qr5inv;
double Ex, Ey, Ez, Exx, Eyy, Ezz, Exy, Exz, Eyz, Ucoul_C, fcoul_Cx, fcoul_Cy, fcoul_Cz;
Ex = 0.0;
Ey = 0.0;
Ez = 0.0;
Exx = 0.0;
Eyy = 0.0;
Ezz = 0.0;
Exy = 0.0;
Exz = 0.0;
Eyz = 0.0;
Ucoul_C = 0.0;
fcoul_Cx = 0.0;
fcoul_Cy = 0.0;
fcoul_Cz = 0.0;

double s1, s2, Exi, Eyi, Ezi, Exxi, Eyyi, Ezzi, Exyi, Exzi, Eyzi;
int neighborscells, neighbors, target_id;
int cell_index;
double Ucoul_shift, rc3inv, smooth_factor, qrc;
double rc6inv, Ulj_cut;
double flj_Cx, flj_Cy, flj_Cz;

if (inside == true) {  
  neighborscells = linkedcell->neighbors1_cells[index];
  for (int i = 0; i < neighborscells; i++) {;
    cell_index = linkedcell->neighbors1_cells_ids[index][i];
    neighbors = linkedcell->atoms_inside_cell[cell_index];
    #pragma omp simd
    for (int j = 0; j < neighbors; j++) {
      target_id = linkedcell->atoms_ids[cell_index][j];
      r_target[0] = moleculeTarget->x[target_id];
      r_target[1] = moleculeTarget->y[target_id];
      r_target[2] = moleculeTarget->z[target_id];
 
      dx = r_probe[0] - r_target[0];
      dy = r_probe[1] - r_target[1];
      dz = r_probe[2] - r_target[2];
      r2 = dx*dx + dy*dy + dz*dz;
      r = sqrt(r2);

      if (r < lj_cutoff) {
        epsilon = moleculeTarget->eps_central[target_id];
        sigma = moleculeTarget->sig_central[target_id]; 
        r2inv = 1.0/r2;
        r6inv = r2inv*r2inv*r2inv;
        lj1 = 4.0*epsilon*pow(sigma,6.0);
        lj2 = lj1*pow(sigma,6.0);
        Ulj = r6inv*(lj2*r6inv - lj1);
        rc6inv = 1.0/pow(lj_cutoff,6);
        Ulj_cut = rc6inv*(lj2*rc6inv - lj1);
        lj3 = 6.0*lj1;
        lj4 = 12.0*lj2;
        flj = r6inv*(lj4*r6inv - lj3)*r2inv;		

        Ulj_C += Ulj - Ulj_cut;
        flj_Cx += flj*dx;
        flj_Cy += flj*dy;
        flj_Cz += flj*dz;
      }

      if (r < coul_cutoff) {
        q = moleculeTarget->q[target_id];
	      rinv = 1.0/r;
	      Ucoul = qC*q*rinv*KCOUL;
        Ucoul_shift = -1.5*qC*q*KCOUL/coul_cutoff + 0.5*qC*q*KCOUL*r2/pow(coul_cutoff,3);
	      r2inv = 1.0/r2;
        fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));

        Ucoul_C += Ucoul + Ucoul_shift;
	      fcoul_Cx += fcoul*dx;
	      fcoul_Cy += fcoul*dy;
        fcoul_Cz += fcoul*dz; 

        r3inv = rinv*r2inv;
        r5inv = r3inv*r2inv;
        rc3inv = 1.0/pow(coul_cutoff,3);
        smooth_factor = (1.0 - r*r2*rc3inv);
        qr3inv = q*r3inv*smooth_factor;
        qr5inv = -3.0*q*r5inv*smooth_factor;
        qrc = -3.0*q*rc3inv*r2inv; 

        Exi = dx * qr3inv;
        Eyi = dy * qr3inv;
        Ezi = dz * qr3inv;

        Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
        Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
        Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

        Exyi = dx*dy*qr5inv + dx*dy*qrc;
        Exzi = dx*dz*qr5inv + dx*dz*qrc;
        Eyzi = dy*dz*qr5inv + dy*dz*qrc;                 
  
        Ex += Exi;
        Ey += Eyi;
        Ez += Ezi;

        Exx += Exxi;
        Eyy += Eyyi;
        Ezz += Ezzi;

        Exy += Exyi;
        Exz += Exzi;
        Eyz += Eyzi;  
      }
    }
  }
  
  neighborscells = linkedcell->neighbors2_cells[index];  
  for (int i = 0; i < neighborscells; i++) {
    cell_index = linkedcell->neighbors2_cells_ids[index][i];
    neighbors = linkedcell->atoms_inside_cell[cell_index];
    #pragma omp simd
    for (int j = 0; j < neighbors; j++) {
      target_id = linkedcell->atoms_ids[cell_index][j];
      r_target[0] = moleculeTarget->x[target_id];
      r_target[1] = moleculeTarget->y[target_id];
      r_target[2] = moleculeTarget->z[target_id];

      dx = r_probe[0] - r_target[0];
      dy = r_probe[1] - r_target[1];
      dz = r_probe[2] - r_target[2];
      r2 = dx*dx + dy*dy + dz*dz;
      r = sqrt(r2);

      if (r < coul_cutoff) {
        q = moleculeTarget->q[target_id];
	      rinv = 1.0/r;
	      Ucoul = qC*q*rinv*KCOUL;
        Ucoul_shift = -1.5*qC*q*KCOUL/coul_cutoff + 0.5*qC*q*KCOUL*r2/pow(coul_cutoff,3); 
        r2inv = 1.0/r2;
        fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));    
	    
        Ucoul_C += Ucoul;
	      fcoul_Cx += fcoul*dx;
	      fcoul_Cy += fcoul*dy;
        fcoul_Cz += fcoul*dz; 

        r3inv = rinv*r2inv;
        r5inv = r3inv*r2inv;
        rc3inv = 1.0/pow(coul_cutoff,3);
        smooth_factor = (1.0 - r*r2*rc3inv);
        qr3inv = q*r3inv*smooth_factor;
        qr5inv = -3.0*q*r5inv*smooth_factor;
        qrc = -3.0*q*rc3inv*r2inv; 

        Exi = dx * qr3inv;
        Eyi = dy * qr3inv;
        Ezi = dz * qr3inv;

        Exxi = qr3inv + dx*dx*qr5inv + dx*dx*qrc;
        Eyyi = qr3inv + dy*dy*qr5inv + dy*dy*qrc;
        Ezzi = qr3inv + dz*dz*qr5inv + dz*dz*qrc;

        Exyi = dx*dy*qr5inv + dx*dy*qrc;
        Exzi = dx*dz*qr5inv + dx*dz*qrc;
        Eyzi = dy*dz*qr5inv + dy*dz*qrc;
  
        Ex += Exi;
        Ey += Eyi;
        Ez += Ezi;

        Exx += Exxi;
        Eyy += Eyyi;
        Ezz += Ezzi;

        Exy += Exyi;
        Exz += Exzi;
        Eyz += Eyzi;
      }  
    }
  }    
}

// nitrogen particles
double rd;
rd = 0.5*gas->d;

// 1 axis X
r_O[0][0] = r_probe[0] + rd;
r_O[0][1] = r_probe[1];
r_O[0][2] = r_probe[2];
// 2 axis X
r_O[1][0] = r_probe[0] - rd;
r_O[1][1] = r_probe[1];
r_O[1][2] = r_probe[2];
// 1 axis Y
r_O[2][0] = r_probe[0];
r_O[2][1] = r_probe[1] + rd;
r_O[2][2] = r_probe[2];
// 2 axis Y
r_O[3][0] = r_probe[0];
r_O[3][1] = r_probe[1] - rd;
r_O[3][2] = r_probe[2];
// 1 axis Z
r_O[4][0] = r_probe[0];
r_O[4][1] = r_probe[1];
r_O[4][2] = r_probe[2] + rd;
// 2 axis Z
r_O[5][0] = r_probe[0];
r_O[5][1] = r_probe[1];
r_O[5][2] = r_probe[2] - rd;

double Ulj_O[6], flj_x[6], flj_y[6], flj_z[6];
double Ucoul_O[6], fcoul_Ox[6], fcoul_Oy[6], fcoul_Oz[6];
double pos_O[3];

for (int k = 0; k < 6; k++) {
  pos_O[0] = r_O[k][0];
  pos_O[1] = r_O[k][1];
  pos_O[2] = r_O[k][2];
  
  linkedcell->calculateIndex(pos_O,index);

  // central particle
  if (index >= linkedcell->Ncells && index < 0) {    
    inside = false;
  } else inside = true;
  
  Ulj_O[k] = 0.0;
  flj_x[k] = 0.0;
  flj_y[k] = 0.0;
  flj_z[k] = 0.0;
  Ucoul_O[k] = 0.0;
  fcoul_Ox[k] = 0.0;
  fcoul_Oy[k] = 0.0;
  fcoul_Oz[k] = 0.0;

  if (inside == true) {
    neighborscells = linkedcell->neighbors1_cells[index];
    for (int i = 0; i < neighborscells; i++) {
      cell_index = linkedcell->neighbors1_cells_ids[index][i];
      neighbors = linkedcell->atoms_inside_cell[cell_index];
      #pragma omp simd
      for (int j = 0; j < neighbors; j++) {
        target_id = linkedcell->atoms_ids[cell_index][j];
        r_target[0] = moleculeTarget->x[target_id];
        r_target[1] = moleculeTarget->y[target_id];
        r_target[2] = moleculeTarget->z[target_id];
    
        dx = pos_O[0] - r_target[0];
        dy = pos_O[1] - r_target[1];
        dz = pos_O[2] - r_target[2];
        r2 = dx*dx + dy*dy + dz*dz;
        r =  sqrt(r2);

        if (r < lj_cutoff) {
          epsilon = moleculeTarget->eps[target_id];
          sigma = moleculeTarget->sig[target_id]; 
          r2inv = 1.0/r2;
          r6inv = r2inv*r2inv*r2inv;
          lj1 = 4.0*epsilon*pow(sigma,6.0);
          lj2 = lj1*pow(sigma,6.0);
          Ulj = r6inv*(lj2*r6inv - lj1);
          rc6inv = 1.0/pow(lj_cutoff,6);
          Ulj_cut = rc6inv*(lj2*rc6inv - lj1);
          lj3 = 6.0*lj1;
          lj4 = 12.0*lj2;
          flj = r6inv*(lj4*r6inv - lj3)*r2inv;
	      
          Ulj_O[k] += Ulj - Ulj_cut;
          flj_x[k] += flj*dx;
          flj_y[k] += flj*dy;
          flj_z[k] += flj*dz;
        }  

        if (r < coul_cutoff) {
          q = moleculeTarget->q[target_id];
          rinv = 1.0/r;
          Ucoul = qO*q*rinv*KCOUL;
          Ucoul_shift = -1.5*qO*q*KCOUL/coul_cutoff + 0.5*qO*q*KCOUL*r2/pow(coul_cutoff,3); 
          r2inv = 1.0/r2;
          fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));
          
          Ucoul_O[k] += Ucoul + Ucoul_shift;
          fcoul_Ox[k] += fcoul*dx;
          fcoul_Oy[k] += fcoul*dy;
          fcoul_Oz[k] += fcoul*dz; 
        }
      }
    }
    
    neighborscells = linkedcell->neighbors2_cells[index];
    for (int i = 0; i < neighborscells; i++) {
      cell_index = linkedcell->neighbors2_cells_ids[index][i];
      neighbors = linkedcell->atoms_inside_cell[cell_index];
      #pragma omp simd
      for (int j = 0; j < neighbors; j++) {
        target_id = linkedcell->atoms_ids[cell_index][j];
        r_target[0] = moleculeTarget->x[target_id];
        r_target[1] = moleculeTarget->y[target_id];
        r_target[2] = moleculeTarget->z[target_id];

        dx = pos_O[0] - r_target[0];
        dy = pos_O[1] - r_target[1];
        dz = pos_O[2] - r_target[2];
        r2 = dx*dx + dy*dy + dz*dz;
        r =  sqrt(r2);  
         
        if (r < coul_cutoff) {
          q = moleculeTarget->q[target_id];
	        rinv = 1.0/r;
	        Ucoul = qO*q*rinv*KCOUL;
          Ucoul_shift = -1.5*qO*q*KCOUL/coul_cutoff + 0.5*qO*q*KCOUL*r2/pow(coul_cutoff,3); 
	        r2inv = 1.0/r2;
          fcoul = Ucoul*r2inv*(1.0 - pow(r/coul_cutoff,3));
                   
          Ucoul_O[k] += Ucoul + Ucoul_shift;
	        fcoul_Ox[k] += fcoul*dx;
	        fcoul_Oy[k] += fcoul*dy;
          fcoul_Oz[k] += fcoul*dz; 
        } 
      }  
    }  
  }
}

double Uind, find[3];
Uind = -0.5 * alpha * (Ex*Ex + Ey*Ey + Ez*Ez);
find[0] = alpha * (Ex*Exx + Ey*Exy + Ez*Exz);
find[1] = alpha * (Ex*Exy + Ey*Eyy + Ez*Eyz);
find[2] = alpha * (Ex*Exz + Ey*Eyz + Ez*Ezz);

double Umol[3], fmolX[3],fmolY[3],fmolZ[3];
Umol[0] = Ulj_O[0] + Ulj_O[1] + Ucoul_O[0] + Ucoul_O[1] + Uind + Ucoul_C + Ulj_C;
Umol[1] = Ulj_O[2] + Ulj_O[3] + Ucoul_O[2] + Ucoul_O[3] + Uind + Ucoul_C + Ulj_C;
Umol[2] = Ulj_O[4] + Ulj_O[5] + Ucoul_O[4] + Ucoul_O[5] + Uind + Ucoul_C + Ulj_C;

double Umin;

Umin = min(Umol[0],min(Umol[1],Umol[2]));

fmolX[0] = flj_x[0] + flj_x[1] + fcoul_Ox[0] + fcoul_Ox[1] + find[0] + fcoul_Cx + flj_Cx;
fmolY[0] = flj_y[0] + flj_y[1] + fcoul_Oy[0] + fcoul_Oy[1] + find[1] + fcoul_Cy + flj_Cy;
fmolZ[0] = flj_z[0] + flj_z[1] + fcoul_Oz[0] + fcoul_Oz[1] + find[2] + fcoul_Cz + flj_Cz;

fmolX[1] = flj_x[2] + flj_x[3] + fcoul_Ox[2] + fcoul_Ox[3] + find[0] + fcoul_Cx + flj_Cx;
fmolY[1] = flj_y[2] + flj_y[3] + fcoul_Oy[2] + fcoul_Oy[3] + find[1] + fcoul_Cy + flj_Cy;
fmolZ[1] = flj_z[2] + flj_z[3] + fcoul_Oz[2] + fcoul_Oz[3] + find[2] + fcoul_Cy + flj_Cz;

fmolX[2] = flj_x[4] + flj_x[5] + fcoul_Ox[4] + fcoul_Ox[5] + find[0] + fcoul_Cx + flj_Cx;
fmolY[2] = flj_y[4] + flj_y[5] + fcoul_Oy[4] + fcoul_Oy[5] + find[1] + fcoul_Cy + flj_Cy;
fmolZ[2] = flj_z[4] + flj_z[5] + fcoul_Oz[4] + fcoul_Oz[5] + find[2] + fcoul_Cz + flj_Cz;

double dU, Z, kBT, T;
Z = 0.0;
T = 500.0;
kBT = BOLTZMANN_K * T * J_TO_eV * eV_TO_KCAL_MOL;

for (int i = 0; i < 3; i++) {
  dU = Umol[i] - Umin;
  Z += exp(-dU/kBT);
}

Up = 0.0;
f[0] = 0.0;
f[1] = 0.0;
f[2] = 0.0;

double w;
for (int i = 0; i < 3; i++) {
  dU = Umol[i] - Umin;
  w = exp(-dU/kBT)/Z;
  Up += w * Umol[i];
  f[0] += w * fmolX[i];
  f[1] += w * fmolY[i];
  f[2] += w * fmolZ[i];
}

return;
}