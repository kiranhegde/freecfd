/************************************************************************
	
	Copyright 2007-2008 Emre Sozer & Patrick Clark Trizila

	Contact: emresozer@freecfd.com , ptrizila@freecfd.com

	This file is a part of Free CFD

	Free CFD is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    Free CFD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    For a copy of the GNU General Public License,
    see <http://www.gnu.org/licenses/>.

*************************************************************************/
#include "commons.h"
#include <iostream>
#include <fstream>

#include <iomanip>
#include <cmath>
#include <mpi.h>
#include <sys/stat.h>
#include <sys/types.h>
using namespace std;


#include "bc.h"
#include "inputs.h"
#include <cgnslib.h>

extern BC bc;

extern string int2str(int number) ;
void write_tec(double time);
void write_vtk(void);
void write_vtk_parallel(void);

void write_output(double time, InputFile input) {
	mkdir("./output",S_IRWXU);
	if (OUTPUT_FORMAT==VTK) {
		// Write vtk output file
		if (Rank==0) write_vtk_parallel();
		write_vtk();
	} else if(OUTPUT_FORMAT==TECPLOT) {
		// Write tecplot output file
		for (int p=0;p<np;++p) {
			if(Rank==p) write_tec(time);
			MPI_Barrier(MPI_COMM_WORLD);
		}
	}
	return;
}

void write_tec(double time) {

	ofstream file;
	string fileName="./output/out"+int2str(timeStep)+".dat";
	// Proc 0 creates the output file and writes variable list
	if (Rank==0) {
		file.open((fileName).c_str(),ios::out); 
		file << "VARIABLES = \"x\", \"y\", \"z\",\"rho\",\"u\",\"v\",\"w\",\"p\",\"Ma\",\"k\",\"omega\" " << endl;
	} else {
		file.open((fileName).c_str(),ios::app);
	}

	// Write header (each proc has its own zone)
	file << "ZONE, T=\"Partition " << Rank << "\"" ;
	file << ", N=" << grid.nodeCount << ", E=" << grid.cellCount << endl;
	file << "DATAPACKING=POINT, ZONETYPE=FEBRICK, SOLUTIONTIME=" << time << endl;

	// Write variables
	map<int,double>::iterator it;
	set<int>::iterator sit;
	double rho_node,p_node,k_node,omega_node;
    Vec3D v_node;
	int count_rho,count_v,count_k,count_omega;
	double Ma;
	for (unsigned int n=0;n<grid.nodeCount;++n) {
		rho_node=0.;v_node=0.;p_node=0.;k_node=0.;omega_node=0.;
		for ( it=grid.node[n].average.begin() ; it != grid.node[n].average.end(); it++ ) {
			if ((*it).first>=0) { // if contribution is coming from a real cell
				rho_node+=(*it).second*grid.cell[(*it).first].rho;
				v_node+=(*it).second*grid.cell[(*it).first].v;
				p_node+=(*it).second*grid.cell[(*it).first].p;
				k_node+=(*it).second*grid.cell[(*it).first].k;
				omega_node+=(*it).second*grid.cell[(*it).first].omega;
			} else { // if contribution is coming from a ghost cell
				rho_node+=(*it).second*grid.ghost[-1*((*it).first+1)].rho;
				v_node+=(*it).second*grid.ghost[-1*((*it).first+1)].v;
				p_node+=(*it).second*grid.ghost[-1*((*it).first+1)].p;
				k_node+=(*it).second*grid.ghost[-1*((*it).first+1)].k;
				omega_node+=(*it).second*grid.ghost[-1*((*it).first+1)].omega;
			}
		}
		
		count_rho=0; count_v=0; count_k=0; count_omega=0;
		for (sit=grid.node[n].bcs.begin();sit!=grid.node[n].bcs.end();sit++) {
			if (bc.region[(*sit)].type==INLET) {
				rho_node=bc.region[(*sit)].rho; count_rho++;
				v_node=bc.region[(*sit)].v; count_v++;
				k_node=bc.region[(*sit)].k; count_k++;
				omega_node=bc.region[(*sit)].omega; count_omega++;
			}
			if (bc.region[(*sit)].type==NOSLIP) {
				if (count_v==0) v_node=0.; count_v++;
			}

			if (count_rho>0) rho_node/=double(count_rho);
			if (count_v>0) v_node/=double(count_v);
			if (count_k>0) k_node/=double(count_k);
			if (count_omega>0) omega_node/=double(count_omega);
		}
		
		Ma=sqrt((v_node.dot(v_node))/(Gamma*(p_node+Pref)/rho_node));
					
		file << setw(16) << setprecision(8) << scientific;
		file << grid.node[n].comp[0] << "\t";
		file << grid.node[n].comp[1] << "\t";
		file << grid.node[n].comp[2] << "\t";
		file << rho_node << "\t" ;
		file << v_node.comp[0] << "\t";
		file << v_node.comp[1] << "\t";
		file << v_node.comp[2] << "\t";
		file << p_node << "\t";
		file << Ma << "\t";
		file << k_node << "\t";
		file << omega_node << "\t";
		file << endl;
	}

	// Write coonnectivity
	for (unsigned int c=0;c<grid.cellCount;++c) {
		if (grid.cell[c].nodeCount==4) {
			file << grid.cell[c].nodes[0]+1 << "\t" ;
			file << grid.cell[c].nodes[2]+1 << "\t" ;
			file << grid.cell[c].nodes[1]+1 << "\t" ;
			file << grid.cell[c].nodes[1]+1 << "\t" ;
			file << grid.cell[c].nodes[3]+1 << "\t" ;
			file << grid.cell[c].nodes[3]+1 << "\t" ;
			file << grid.cell[c].nodes[3]+1 << "\t" ;
			file << grid.cell[c].nodes[3]+1 << "\t" ;
		}
		else if (grid.cell[c].nodeCount==5) {
			file << grid.cell[c].nodes[0]+1 << "\t" ;
			file << grid.cell[c].nodes[1]+1 << "\t" ;
			file << grid.cell[c].nodes[2]+1 << "\t" ;
			file << grid.cell[c].nodes[3]+1 << "\t" ;
			file << grid.cell[c].nodes[4]+1 << "\t" ;
			file << grid.cell[c].nodes[4]+1 << "\t" ;
			file << grid.cell[c].nodes[4]+1 << "\t" ;
			file << grid.cell[c].nodes[4]+1 << "\t" ;
		}
		else if (grid.cell[c].nodeCount==6) {
			file << grid.cell[c].nodes[0]+1 << "\t" ;
			file << grid.cell[c].nodes[1]+1 << "\t" ;
			file << grid.cell[c].nodes[2]+1 << "\t" ;
			file << grid.cell[c].nodes[2]+1 << "\t" ;
			file << grid.cell[c].nodes[3]+1 << "\t" ;
			file << grid.cell[c].nodes[4]+1 << "\t" ;
			file << grid.cell[c].nodes[5]+1 << "\t" ;
			file << grid.cell[c].nodes[5]+1 << "\t" ;
		} else if (grid.cell[c].nodeCount==8) {
			for (unsigned int i=0;i<8;++i) {
				file << grid.cell[c].nodes[i]+1 << "\t";
			}
		}
		file << endl;
	}
	
	file.close();

}

void write_vtk(void) {

	string filePath="./output/"+int2str(timeStep);
	string fileName=filePath+"/proc"+int2str(Rank)+".vtu";
		
	mkdir(filePath.c_str(),S_IRWXU);
	
	ofstream file;
	file.open((fileName).c_str(),ios::out);
	file << "<?xml version=\"1.0\"?>" << endl;
	file << "<VTKFile type=\"UnstructuredGrid\">" << endl;
	file << "<UnstructuredGrid>" << endl;
	file << "<Piece NumberOfPoints=\"" << grid.nodeCount << "\" NumberOfCells=\"" << grid.cellCount << "\">" << endl;
	file << "<Points>" << endl;
	file << "<DataArray NumberOfComponents=\"3\" type=\"Float32\" format=\"ascii\" >" << endl;
	for (unsigned int n=0;n<grid.nodeCount;++n) {
		for (unsigned int i=0; i<3; ++i) file<< setw(16) << setprecision(8) << scientific << grid.node[n].comp[i] << endl;
	}
	file << "</DataArray>" << endl;
	file << "</Points>" << endl;
	file << "<Cells>" << endl;
	
	file << "<DataArray Name=\"connectivity\" type=\"Int32\" format=\"ascii\" >" << endl;
	for (unsigned int c=0;c<grid.cellCount;++c) {
		for (unsigned int n=0;n<grid.cell[c].nodeCount;++n) {
			file << grid.cell[c].nodes[n] << "\t";
		}
		file << endl;
	}
	
	file << "</DataArray>" << endl;
	file << "<DataArray Name=\"offsets\" type=\"Int32\" format=\"ascii\" >" << endl;
	int offset=0;
	for (unsigned int c=0;c<grid.cellCount;++c) {
		offset+=grid.cell[c].nodeCount;
		file << offset << "\t";
	}
	file << endl;
	file << "</DataArray>" << endl;
			
	file << "<DataArray Name=\"types\" type=\"UInt8\" format=\"ascii\" >" << endl;
	for (unsigned int c=0;c<grid.cellCount;++c) {
		if (grid.cell[c].nodeCount==4) file << "10\t"; // Tetra
		if (grid.cell[c].nodeCount==8) file << "12\t"; // Hexa
		if (grid.cell[c].nodeCount==6) file << "13\t"; // Prism (Wedge)
		if (grid.cell[c].nodeCount==5) file << "14\t"; // Pyramid (Wedge)
	}
	file << endl;
	file << "</DataArray>" << endl;;
	
	file << "</Cells>" << endl;

	file << "<CellData Scalars=\"Density\" Vectors=\"Velocity\" format=\"ascii\">" << endl;
	file << "<DataArray Name=\"Density\" type=\"Float32\" format=\"ascii\" >" << endl;
	for (unsigned int c=0;c<grid.cellCount;++c) file << setw(16) << setprecision(8) << scientific << grid.cell[c].rho << endl;
	file << "</DataArray>" << endl;
	file << "<DataArray Name=\"Velocity\" NumberOfComponents=\"3\" type=\"Float32\" format=\"ascii\" >" << endl;
	for (unsigned int c=0;c<grid.cellCount;++c) for (unsigned int i=0;i<3;++i) file << setw(16) << setprecision(8) << scientific << grid.cell[c].v.comp[i] << endl;
	file << "</DataArray>" << endl;
	file << "<DataArray Name=\"Pressure\" type=\"Float32\" format=\"ascii\" >" << endl;
	for (unsigned int c=0;c<grid.cellCount;++c) file << setw(16) << setprecision(8) << scientific << grid.cell[c].p << endl;
	file << "</DataArray>" << endl;
	file << "</CellData>" << endl;
	
	file << "</Piece>" << endl;
	file << "</UnstructuredGrid>" << endl;
	file << "</VTKFile>" << endl;
	file.close();
	
}

void write_vtk_parallel(void) {

	string filePath="./output/"+int2str(timeStep);
	string fileName=filePath+"/out"+int2str(timeStep)+".pvtu";
		
	mkdir("./output",S_IRWXU);
	mkdir(filePath.c_str(),S_IRWXU);
	
	ofstream file;
	file.open((fileName).c_str(),ios::out);
	file << "<?xml version=\"1.0\"?>" << endl;
	file << "<VTKFile type=\"PUnstructuredGrid\">" << endl;
	file << "<PUnstructuredGrid GhostLevel=\"0\">" << endl;
	file << "<PPoints>" << endl;
	file << "<DataArray NumberOfComponents=\"3\" type=\"Float32\" format=\"ascii\" />" << endl;
	file << "</PPoints>" << endl;

	file << "<PCellData Scalars=\"Density\" Vectors=\"Velocity\" format=\"ascii\">" << endl;
	file << "<DataArray Name=\"Density\" type=\"Float32\" format=\"ascii\" />" << endl;
	file << "<DataArray Name=\"Velocity\" NumberOfComponents=\"3\" type=\"Float32\" format=\"ascii\" />" << endl;
	file << "<DataArray Name=\"Pressure\" type=\"Float32\" format=\"ascii\" />" << endl;
	file << "</PCellData>" << endl;
	for (int p=0;p<np;++p) file << "<Piece Source=\"proc" << int2str(p) << ".vtu\" />" << endl;
	file << "</PUnstructuredGrid>" << endl;
	file << "</VTKFile>" << endl;
	file.close();
	
}

