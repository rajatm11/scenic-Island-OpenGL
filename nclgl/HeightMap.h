#pragma once
#include  <string>
#include "Mesh.h"
#include <vector>
class HeightMap: public  Mesh
{
public:
	HeightMap(const  std::string& name); 
	~HeightMap(void) {};
	Vector3  GetHeightmapSize()  const { 
		return  heightmapSize; 
	}
	
protected:
	Vector3  heightmapSize;
	int  V3[10000];
};

