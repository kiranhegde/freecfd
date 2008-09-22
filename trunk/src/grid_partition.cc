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
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <iomanip>
#include<cmath>
using namespace std;

#include <parmetis.h>


#include "grid.h"

extern Grid grid;
extern int np, Rank;
extern GridRawData raw;

int Grid::Partition() {

	// Initialize the partition sizes
	// This is just a simple manual partitioning to be able to use parmetis afterwards
	cellCount=floor(double(globalCellCount)/double(np));
	int baseCellCount=cellCount;
	unsigned int offset=Rank*cellCount;
	if (Rank==np-1) cellCount=cellCount+globalCellCount-np*cellCount;

	//Implementing Parmetis
	/* ParMETIS_V3_PartMeshKway(idxtype *elmdist, idxtype *eptr, idxtype *eind, idxtype *elmwgt, int *wgtflag, int *numflag, int *ncon, int * ncommonnodes, int *nparts, float *tpwgts, float *ubvec, int *options, int *edgecut, idxtype *part, MPI_Comm) */

	/*  Definining variables
	elmdist- look into making the 5 arrays short int (for performance
		on 64 bit arch)
	eptr- like xadf
	eind- like adjncy
	elmwgt- null (element weights)
	wgtflag- 0 (no weights, can take value of 0,1,2,3 see documentation)
	numflag- 0 C-style numbers, 1 Fortran-style numbers
	ncon- 1  ( # of constraints)
	ncommonnodes- 4 ( we can probably put this to 3)
	nparts- # of processors (Note: BE CAREFUL if != to # of proc)
	tpwgts- 
	ubvec-  (balancing constraints,if needed 1.05 is a good value)
	options- [0 1 15] for default
	edgecut- output, # of edges cut (measure of communication)
	part- output, where our elements should be
	comm- most likely MPI_COMM_WORLD
	*/

	idxtype elmdist[np+1];
	idxtype *eptr;
	eptr = new idxtype[cellCount+1];
	idxtype *eind;
	int eindSize=0;
	if ((offset+cellCount)==globalCellCount) {
		eindSize=raw.cellConnectivity.size()-raw.cellConnIndex[offset];
	}
	else {
		eindSize=raw.cellConnIndex[offset+cellCount]-raw.cellConnIndex[offset]+1;
	}
	eind = new idxtype[eindSize];
	idxtype* elmwgt = NULL;
	int wgtflag=0; // no weights associated with elem or edges
	int numflag=0; // C-style numbering
	int ncon=1; // # of weights or constraints
	int ncommonnodes; ncommonnodes=3; // set to 3 for tetrahedra or mixed type

	float tpwgts[np];
	for (unsigned int p=0; p<np; ++p) tpwgts[p]=1./float(np);
	float ubvec=1.02;
	int options[3]; // default values for timing info set 0 -> 1

	options[0]=0; options[1]=1; options[2]=15;
	int edgecut ; // output
	idxtype* part = new idxtype[cellCount];

	for (unsigned int p=0;p<np;++p) elmdist[p]=p*floor(globalCellCount/np);
	elmdist[np]=globalCellCount;// Note this is because #elements mod(np) are all on last proc

	for (unsigned int c=0; c<cellCount;++c) {
		eptr[c]=raw.cellConnIndex[offset+c]-raw.cellConnIndex[offset];
	}
	if ((offset+cellCount)==globalCellCount) {
		eptr[cellCount]=raw.cellConnectivity.size()-raw.cellConnIndex[offset];
	} else {
		eptr[cellCount]=raw.cellConnIndex[offset+cellCount]-raw.cellConnIndex[offset];
	}

	for (unsigned int i=0; i<eindSize; ++i) {
		eind[i]=raw.cellConnectivity[raw.cellConnIndex[offset]+i];
	}

	ompi_communicator_t* commWorld=MPI_COMM_WORLD;

	ParMETIS_V3_PartMeshKway(elmdist,eptr,eind, elmwgt,
	                         &wgtflag, &numflag, &ncon, &ncommonnodes,
	                         &np, tpwgts, &ubvec, options, &edgecut,
	                         part,&commWorld) ;
	
	delete[] eptr;
	delete[] eind;

	// Distribute the part list to each proc
	// Each proc has an array of length globalCellCount which says the processor number that cell belongs to [cellMap]
	int recvCounts[np];
	int displs[np];
	for (int p=0;p<np;++p) {
		recvCounts[p]=baseCellCount;
		displs[p]=p*baseCellCount;
	}
	recvCounts[np-1]=baseCellCount+globalCellCount-np*baseCellCount;
	
	raw.cellMap.resize(globalCellCount);
	//cellMap of a cell returns which processor it is assigned to
	MPI_Allgatherv(part,cellCount,MPI_INT,&raw.cellMap[0],recvCounts,displs,MPI_INT,MPI_COMM_WORLD);

	// Find new local cellCount after ParMetis distribution
	cellCount=0.;
	int otherCellCounts[np]; 
	for (unsigned int p=0;p<np;p++) otherCellCounts[p]=0; 
	
	for (unsigned int c=0;c<globalCellCount;++c) {
		otherCellCounts[raw.cellMap[c]]+=1;
		if (raw.cellMap[c]==Rank) ++cellCount;
	}
	cout << "[I Rank=" << Rank << "] Number of Cells= " << cellCount << endl;

} // end Grid::Partition





