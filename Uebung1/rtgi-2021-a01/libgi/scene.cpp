#include "scene.h"

#include "color.h"
#include "util.h"
#include "wavefront-rt.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <filesystem>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>

// debug
// #include <png++/png.hpp>

#ifdef RTGI_WAND7
#include <MagickWand/MagickWand.h>
#else
#include <wand/MagickWand.h>
#endif

using namespace glm;
using namespace std;

inline vec3 to_glm(const aiVector3D& v) { return vec3(v.x, v.y, v.z); }

static bool verbose_scene = false;

void magickwand_error(MagickWand *wand, bool crash) {
	char *description;
	ExceptionType severity;
	description=MagickGetException(wand,&severity);
	cerr << (GetMagickModule()) << ": " << description << endl;
	MagickRelinquishMemory(description);
	if (crash)
		exit(1);
}

texture* load_image3f(const std::filesystem::path &path, bool crash_on_error) {
	if (verbose_scene) cout << "loading texture " << path << endl;
	MagickWandGenesis();
	MagickWand *img = NewMagickWand();
	int status = MagickReadImage(img, path.c_str());
	if (status == MagickFalse) {
		magickwand_error(img, crash_on_error);
		return nullptr;
	}
	MagickFlipImage(img);
	texture *tex = new texture;
	tex->name = path;
	tex->path = path;
	tex->w = MagickGetImageWidth(img);
	tex->h = MagickGetImageHeight(img);
	tex->texel = new vec3[tex->w*tex->h];
	MagickExportImagePixels(img, 0, 0, tex->w, tex->h, "RGB", FloatPixel, (void*)tex->texel);
	#pragma omp parallel for
	for (int i = 0; i < tex->w*tex->h; ++i)
		tex->texel[i] = pow(tex->texel[i], vec3(2.2f, 2.2f, 2.2f));
	DestroyMagickWand(img);
	MagickWandTerminus();
	return tex;
}

texture* load_hdr_image3f(const std::filesystem::path &path) {
	cout << "loading hdr texture from floats-file " << path << endl;
	ifstream in;
	in.open(path, ios::in | ios::binary);
	if (!in.is_open())
		throw runtime_error("Cannot open file '" + path.string() + "' for hdr floats texture.");
	texture *tex = new texture;
	tex->name = path;
	tex->path = path;
	in.read(((char*)&tex->w), sizeof(int));
	in.read(((char*)&tex->h), sizeof(int));
	tex->texel = new vec3[tex->w * tex->h];
	in.read(((char*)tex->texel), tex->w * tex->h * sizeof(vec3));
	if (!in.good())
		throw runtime_error("Error loading data from '" + path.string() + "' for hdr floats texture.");
	return tex;
}

void scene::add(const filesystem::path& path, const std::string &name, const mat4 &trafo) {
    // load from disk
    Assimp::Importer importer;
//     std::cout << "Loading: " << path << "..." << std::endl;
	unsigned int flags = aiProcess_Triangulate | aiProcess_GenNormals;  // | aiProcess_FlipUVs  // TODO assimp
    const aiScene* scene_ai = importer.ReadFile(path.string(), flags);
    if (!scene_ai) // handle error
        throw std::runtime_error("ERROR: Failed to load file: " + path.string() + "!");

	// todo: store indices prior to adding anything to allow "transform-last"


	// load materials
	unsigned material_offset = materials.size();
    for (uint32_t i = 0; i < scene_ai->mNumMaterials; ++i) {
		::material material;
        aiString name_ai;
		aiColor3D col;
		auto mat_ai = scene_ai->mMaterials[i];
        mat_ai->Get(AI_MATKEY_NAME, name_ai);
		if (name != "") material.name = name + "/" + name_ai.C_Str();
		else            material.name = name_ai.C_Str();
		
		vec3 kd(0), ks(0), ke(0);
		float tmp;
		if (mat_ai->Get(AI_MATKEY_COLOR_DIFFUSE,  col) == AI_SUCCESS) kd = vec4(col.r, col.g, col.b, 1.0f);
		if (mat_ai->Get(AI_MATKEY_COLOR_SPECULAR, col) == AI_SUCCESS) ks = vec4(col.r, col.g, col.b, 1.0f);
		if (mat_ai->Get(AI_MATKEY_COLOR_EMISSIVE, col) == AI_SUCCESS) ke = vec4(col.r, col.g, col.b, 1.0f);
		if (mat_ai->Get(AI_MATKEY_SHININESS,      tmp) == AI_SUCCESS) material.roughness = roughness_from_exponent(tmp);
		if (mat_ai->Get(AI_MATKEY_REFRACTI,       tmp) == AI_SUCCESS) material.ior = tmp;
		if (material.ior == 1.0f) material.ior = 1.3;
		if (luma(kd) > 1e-4) material.albedo = kd;
		else                 material.albedo = ks;
		material.albedo = pow(material.albedo, vec3(2.2f, 2.2f, 2.2f));
		material.emissive = ke;
		
		if (mat_ai->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path_ai;
			mat_ai->GetTexture(aiTextureType_DIFFUSE, 0, &path_ai);
			filesystem::path p = path.parent_path() / path_ai.C_Str();
			material.albedo_tex = load_image3f(p);
			textures.push_back(material.albedo_tex);
		}

	
		materials.push_back(material);
	}

    // load meshes
    for (uint32_t i = 0; i < scene_ai->mNumMeshes; ++i) {
        const aiMesh *mesh_ai = scene_ai->mMeshes[i];
		uint32_t material_id = scene_ai->mMeshes[i]->mMaterialIndex + material_offset;
		uint32_t index_offset = vertices.size();
		std::string object_name = mesh_ai->mName.C_Str();
		objects.push_back({object_name, (unsigned)triangles.size(), (unsigned)(triangles.size()+mesh_ai->mNumFaces), material_id});
		
		for (uint32_t i = 0; i < mesh_ai->mNumVertices; ++i) {
			vertex vertex;
			vertex.pos = to_glm(mesh_ai->mVertices[i]);
			vertex.norm = to_glm(mesh_ai->mNormals[i]);
			if (mesh_ai->HasTextureCoords(0))
				vertex.tc = vec2(to_glm(mesh_ai->mTextureCoords[0][i]));
			else
				vertex.tc = vec2(0,0);
			vertices.push_back(vertex);
			scene_bounds.grow(vertex.pos);
		}
 
		for (uint32_t i = 0; i < mesh_ai->mNumFaces; ++i) {
			const aiFace &face = mesh_ai->mFaces[i];
			if (face.mNumIndices == 3) {
				triangle triangle;
				triangle.a = face.mIndices[0] + index_offset;
				triangle.b = face.mIndices[1] + index_offset;
				triangle.c = face.mIndices[2] + index_offset;
				triangle.material_id = material_id;
				triangles.push_back(triangle);
			}
			else
				std::cout << "WARN: Mesh: skipping non-triangle [" << face.mNumIndices << "] face (that the ass imp did not triangulate)!" << std::endl;
		}
	}
}
	

scene::~scene() {
	delete rt;
	for (auto *x : textures)
		delete x;
}

vec3 scene::normal(const triangle &tri) const {
	const vec3 &a = vertices[tri.a].pos;
	const vec3 &b = vertices[tri.b].pos;
	const vec3 &c = vertices[tri.c].pos;
	vec3 e1 = normalize(b-a);
	vec3 e2 = normalize(c-a);
	return cross(e1, e2);
}

vec3 scene::sample_texture(const triangle_intersection &is, const triangle &tri, const texture *tex) const {
	vec2 tc = (1.0f-is.beta-is.gamma)*vertices[tri.a].tc + is.beta*vertices[tri.b].tc + is.gamma*vertices[tri.c].tc;
	return (*tex)(tc);
}
	
void scene::release_rt() {
	rt = nullptr;
	single_rt = nullptr;
	batch_rt = nullptr;
}

void scene::use(ray_tracer *new_rt) {
	delete rt;
	rt = new_rt;
	single_rt = dynamic_cast<individual_ray_tracer*>(rt);
	batch_rt = dynamic_cast<wf::batch_ray_tracer*>(rt);
}




/////


/////

