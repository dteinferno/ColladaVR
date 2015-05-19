#include "colladainterface.h"

char array_types[7][15] = { "float_array", "int_array", "bool_array", "Name_array",
"IDREF_array", "SIDREF_array", "token_array" };

void ColladaInterface::readGeometries(std::vector<ColGeom>* v, const char* filename) {

	TiXmlElement *mesh, *polylist, *vertices, *vinput, *input, *source;
	std::string texture_name;
	unsigned int lName;
	std::string attrib_name, source_name;
	int prim_count, num_indices;

	// Create document and load COLLADA file
	TiXmlDocument doc(filename);
	doc.LoadFile();
	TiXmlElement* geometry =
		doc.RootElement()->FirstChildElement("library_geometries")->FirstChildElement("geometry");

	// Iterate through geometry elements 
	while (geometry != NULL) {

		// Create new geometry
		ColGeom data;

		// Get the geometry name
		data.name = geometry->Attribute("name");

		// Iterate through mesh elements 
		mesh = geometry->FirstChildElement("mesh");
		while (mesh != NULL) {

			// Extract vertex, normal, and texture information from the polylist
			polylist = mesh->FirstChildElement("polylist");

			// Determine the texture name			
			if (polylist->Attribute("material") != NULL)
			{
				texture_name = polylist->Attribute("material");
				lName = texture_name.size();
				texture_name = texture_name.erase(lName - 9, lName);
				data.texture = texture_name;
			}
			else
			{
				data.texture = "NONE";
			}
			

			// Detemine the number of elements and set the primitive type
			polylist->QueryIntAttribute("count", &prim_count);
			data.primitive = GL_TRIANGLES;
			num_indices = prim_count * 3;
			data.index_count = num_indices;

			// Iterate through input elements
			input = polylist->FirstChildElement("input");
			while (input != NULL) {

				attrib_name = input->Attribute("semantic");
				if (std::string("VERTEX") == attrib_name)
				{
					vertices = mesh->FirstChildElement("vertices");
					vinput = vertices->FirstChildElement("input");
					source_name = vinput->Attribute("source");
					source_name = source_name.erase(0, 1);
				}
				else
				{
					source_name = input->Attribute("source");
					source_name = source_name.erase(0, 1);
				}
				source = mesh->FirstChildElement("source");

				// Iterate through source elements 
				while (source != NULL) {
					if (std::string(source->Attribute("id")) == source_name) {
						data.map[attrib_name] = readSource(source);
					}

					source = source->NextSiblingElement("source");
				}

				input = input->NextSiblingElement("input");
			}

			// Read the index values
			char* text = (char*)(polylist->FirstChildElement("p")->GetText());
			char *next_token = NULL;

			// Allocate memory for indices and then get the data
			if ((int)data.map["TEXCOORD"].size > 0)
			{
				data.indices = (unsigned short*)malloc(num_indices * 3 * sizeof(unsigned short));
				data.indices[0] = (unsigned short)atoi(strtok_s(text, " ", &next_token));
				for (int index = 1; index < 3 * num_indices; index++) {
					data.indices[index] = (unsigned short)atoi(strtok_s(NULL, " ", &next_token));
				}
			}
			else
			{
				data.indices = (unsigned short*)malloc(num_indices * 2 * sizeof(unsigned short));
				data.indices[0] = (unsigned short)atoi(strtok_s(text, " ", &next_token));
				for (int index = 1; index < 2 * num_indices; index++) {
					data.indices[index] = (unsigned short)atoi(strtok_s(NULL, " ", &next_token));
				}
			}
			
			mesh = mesh->NextSiblingElement("mesh");
		}

		v->push_back(data);

		geometry = geometry->NextSiblingElement("geometry");
	}
}

void ColladaInterface::readTransformations(std::vector<ColTrans>* v, const char* filename) {

	TiXmlElement *node, *translate, *scale;
	char * transtext, *scaletext;
	char *trans_next_token = NULL;
	char *scale_next_token = NULL;

	// Create new transformation
	ColTrans data;

	// Create document and load COLLADA file
	TiXmlDocument doc(filename);
	doc.LoadFile();
	TiXmlElement* visual_scene =
		doc.RootElement()->FirstChildElement("library_visual_scenes")->FirstChildElement("visual_scene");

	// Iterate through node elements 
	node = visual_scene->FirstChildElement("node");
	while (node != NULL) {

		// Extract the object name
		data.name = node->Attribute("name");

		// Extract the transformation values
		translate = node->FirstChildElement("translate");
		transtext = (char*)(translate->GetText()); // Read array values
		data.trans_data = (float *) malloc(3 * sizeof(float));

		// Read the float values
		((float*)data.trans_data)[0] = atof(strtok_s(transtext, " ", &trans_next_token));
		for (unsigned int index = 1; index<3; index++) {
			((float*)data.trans_data)[index] = atof(strtok_s(NULL, " ", &trans_next_token));
		}

		// Extract the scale values
		scale = node->FirstChildElement("scale");
		scaletext = (char*)(scale->GetText()); // Read array values
		data.scale_data = (float *)malloc(3 * sizeof(float));

		// Read the float values
		((float*)data.scale_data)[0] = atof(strtok_s(scaletext, " ", &scale_next_token));
		for (unsigned int index = 1; index<3; index++) {
			((float*)data.scale_data)[index] = atof(strtok_s(NULL, " ", &scale_next_token));
		}

		v->push_back(data);

		node = node->NextSiblingElement("node");
	}
}

void ColladaInterface::readImages(std::vector<ColIm>* v, const char* filename) {

	TiXmlElement *imagename;
	unsigned im_name;
	char* text;

	// Create new texture image
	ColIm data;

	// Create document and load COLLADA file
	TiXmlDocument doc(filename);
	doc.LoadFile();
	TiXmlElement* images =
		doc.RootElement()->FirstChildElement("library_images")->FirstChildElement("image");

	while (images != NULL)
	{

		// Extract the texture label
		data.imagename = images->Attribute("name");

		// Read the texture filename
		imagename = images->FirstChildElement("init_from");
		text = (char*)(imagename->GetText());
		data.imageloc = text;

		v->push_back(data);
		images = images->NextSiblingElement("image");
	}

}

void ColladaInterface::readTextures(std::vector<ColTex>* v, const char* filename) {

	TiXmlElement *texfname, *color;
	char * text;
	char *next_token = NULL;

	std::string texture_name;
	unsigned int lName;
	
	// Create new texture image
	ColTex data;

	// Create document and load COLLADA file
	TiXmlDocument doc(filename);
	doc.LoadFile();
	TiXmlElement* textures =
		doc.RootElement()->FirstChildElement("library_effects")->FirstChildElement("effect");

	while (textures != NULL) {

		// Extract the texture label
		texture_name = textures->Attribute("id");
		lName = texture_name.size();
		texture_name = texture_name.erase(lName - 7, lName);
		data.name = texture_name;

		// Extract the texture filename
		
		if (textures->FirstChildElement("profile_COMMON")->FirstChildElement("newparam") != NULL)
		{
			texfname = textures->FirstChildElement("profile_COMMON")->FirstChildElement("newparam")->FirstChildElement("surface")->FirstChildElement("init_from");
			data.texfname = texfname->GetText();
		}
		else
		{
			data.texfname = "";
		}
		
		// Get out the color
		color = textures->FirstChildElement("profile_COMMON")->FirstChildElement("technique")->FirstChildElement("phong")->FirstChildElement("specular")->FirstChildElement("color");
		text = (char*)(color->GetText()); // Read array values
		data.RGB = (float *)malloc(3 * sizeof(float));

		// Read the float values
		((float*)data.RGB)[0] = atof(strtok_s(text, " ", &next_token));
		for (unsigned int index = 1; index<3; index++) {
			((float*)data.RGB)[index] = atof(strtok_s(NULL, " ", &next_token));
		}


		v->push_back(data);

		textures = textures->NextSiblingElement("effect");
	}
}

void ColladaInterface::freeGeometries(std::vector<ColGeom>* v) {

	std::vector<ColGeom>::iterator geom_it;
	SourceMap::iterator map_it;

	for (geom_it = v->begin(); geom_it < v->end(); geom_it++) {
		// Deallocate index data
		free(geom_it->indices);

		// Deallocate array data in each map value
		for (map_it = geom_it->map.begin(); map_it != geom_it->map.end(); map_it++) {
			free((*map_it).second.data);
		}

		// Erase the current ColGeom from the vector
		v->erase(geom_it);
	}
}

SourceData readSource(TiXmlElement* source) {

	SourceData source_data;
	TiXmlElement *array;
	char* text;
	char *next_token = NULL;
	unsigned int num_vals, stride;
	int check;

	for (int i = 0; i<7; i++) {
		array = source->FirstChildElement(array_types[i]);
		if (array != NULL) {

			// Find number of values
			array->QueryUnsignedAttribute("count", &num_vals);
			source_data.size = (int) num_vals;

			// Find stride
			check = source->FirstChildElement("technique_common")->FirstChildElement("accessor")->QueryUnsignedAttribute("stride", &stride);
			if (check != TIXML_NO_ATTRIBUTE)
				source_data.stride = stride;
			else
				source_data.stride = 1;

			// Read array values
			text = (char*)(array->GetText());

			// Initialize mesh data according to data type
			switch (i) {

				// Array of floats
			case 0:
				source_data.type = GL_FLOAT;
				source_data.size *= sizeof(float);
				source_data.data = malloc(num_vals * sizeof(float));

				// Read the float values
				((float*)source_data.data)[0] = atof(strtok_s(text, " ",&next_token));
				for (unsigned int index = 1; index<num_vals; index++) {
					((float*)source_data.data)[index] = atof(strtok_s(NULL, " ", &next_token));
				}
				break;

				// Array of integers
			case 1:
				source_data.type = GL_INT;
				source_data.size *= sizeof(int);
				source_data.data = malloc(num_vals * sizeof(int));

				// Read the int values
				((int*)source_data.data)[0] = atof(strtok_s(text, " ", &next_token));
				for (unsigned int index = 1; index<num_vals; index++) {
					((int*)source_data.data)[index] = atof(strtok_s(NULL, " ", &next_token));
				}
				break;

				// Other
			default:
				std::cout << "Collada Reader doesn't support mesh data in this format" << std::endl;
				break;
			}
		}
	}
	return source_data;
}
