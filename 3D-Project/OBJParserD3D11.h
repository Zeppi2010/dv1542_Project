#pragma once  
#include <string>  
#include <vector>  
#include <unordered_map>  
#include <fstream>


#include <d3d11_4.h>  
#include <DirectXMath.h>  

#include "MeshD3D11.h"  
#include "ShaderResourceTextureD3D11.h"  
#include "Vertex.h"

class OBJParserD3D11  
{  
private:  
   struct ParsedMaterial  
   {  
       std::string mapKd;  
       std::string mapKs;
       std::string mapKa;  
       float shininess = 0;  
   };  

   struct ParseData  
   {  
       std::vector<DirectX::XMFLOAT3> positions;  
       std::vector<DirectX::XMFLOAT3> normals;  
       std::vector<DirectX::XMFLOAT2> texCoords;  

       std::vector<float> vertexData;  
       std::vector<uint32_t> indexData;  

       std::unordered_map<std::string, uint32_t> uniqueVertices;  

       std::unordered_map<std::string, ParsedMaterial> materials;  
       std::string currentMaterial;  
       size_t currentSubMeshStart = 0;  

       std::vector<MeshData::SubMeshInfo> subMeshes;  
   };  

   std::unordered_map<std::string, ShaderResourceTextureD3D11*> loadedTextures;

   std::unordered_map<std::string, MeshD3D11*> loadedMeshes;

   std::string baseDirectory;

   float ParseFloat(const std::string& str, size_t& pos);
   int ParseInt(const std::string& str, size_t& pos);
   std::string ParseString(const std::string& str, size_t& pos);

   void ParsePosition(const std::string& line, ParseData& data);
   void ParseUV(const std::string& line, ParseData& data);
   void ParseNormal(const std::string& line, ParseData& data);
   void ParseFace(const std::string& line, ParseData& data);

   void ParseMTLFile(const std::string& mtlFilePath, ParseData& data);

   void FinilzeSubMesh(ID3D11Device* device, ParseData& data);

   ID3D11ShaderResourceView* GetOrLoadTexture(ID3D11Device* device, const std::string& texturePath);

   ID3D11ShaderResourceView* GetDefaultTexture(ID3D11Device* device);
   ShaderResourceTextureD3D11* defaultTexture = nullptr;

public:
	OBJParserD3D11() = default;
	~OBJParserD3D11();

	void SetBaseDirectory(const std::string& directory);

	MeshD3D11* LoadMesh(ID3D11Device* device, const std::string& objFilePath);
	
};  