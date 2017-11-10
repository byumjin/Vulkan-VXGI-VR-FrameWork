#pragma once

#include "../core/Common.h"
#include "../assets/Geometry.h"

#include "../assets/Material.h"
#include "../assets/Texture.h"

#include "Actor.h"

class Object : public Actor
{
public:
	Object();
	~Object();

	void initiation(std::string objectNameParam, Geo* geoParam)
	{
		objectName = objectNameParam;
		geo = geoParam;
	}

	void addTexture(Texture* tex)
	{
		textures.push_back(tex);
	}

	void connectMaterial(Material* mat)
	{
		material = mat;
	}

	Geo* geo;
	Material* material;
	std::vector<Texture*> textures;
	std::string objectName;

private:


};
