/************************************************************************
	
	Copyright 2007-2009 Emre Sozer & Patrick Clark Trizila

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
#ifndef PROBE_H
#define PROBE_H

#include <fstream>
#include <mpi.h>
using namespace std;
#include "vec3d.h"

class Probe {
	public:
		int id;
		int Rank;
		int nearestCell;
		Vec3D coord;
		string fileName;
};

class BoundaryFlux {
	public:
		int bc;
		string fileName;
};

#endif