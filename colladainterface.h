#ifndef COLLADAINTERFACE_H
#define COLLADAINTERFACE_H

#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <iterator>
#include "GL/glew.h"
#include "tinyxml/tinyxml.h"

struct SourceData {
	GLenum type;
	unsigned int size;
	unsigned int stride;
	void* data;
};

typedef std::map<std::string, SourceData> SourceMap;

struct ColGeom {
	std::string name;
	SourceMap map;
	GLenum primitive;
	int index_count;
	unsigned short* indices;
	std::string texture;
};

struct ColTrans {
	std::string name;
	float* trans_data;
	float* scale_data;
};

struct ColIm {
	std::string imagename;
	std::string imageloc;
};

struct ColTex {
	std::string name;
	std::string texfname;
	float* RGB;
};

SourceData readSource(TiXmlElement*);

class ColladaInterface {

public:
	ColladaInterface() {};
	static void readGeometries(std::vector<ColGeom>*, const char*);
	static void readTransformations(std::vector<ColTrans>*, const char*);
	static void readImages(std::vector<ColIm>*, const char*);
	static void readTextures(std::vector<ColTex>*, const char*);
	static void freeGeometries(std::vector<ColGeom>*);
};

#endif

