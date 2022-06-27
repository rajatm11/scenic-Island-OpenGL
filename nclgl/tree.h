#pragma once
#include "Vector3.h"

class tree
{
public:
	tree() {} 
	tree(const  Vector3& position) {
		this->position = position;
	
	}

	~tree(void) {};

	Vector3   GetPosition()  const { return  position; }
	void      SetPosition(const  Vector3& val) { position = val; }

	


protected:
	Vector3   position;
	
};

