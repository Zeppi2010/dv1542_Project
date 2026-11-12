#include "OBJParserD3D11.h"

float OBJParserD3D11::ParseFloat(const std::string& str, size_t& pos)
{
	while (pos < str.size() && std::isspace(str[pos]))
	{
		pos++;
	}

	size_t start = pos;

	while (pos < str.size() && !std::isspace(str[pos]))
	{
		pos++;
	}
	return std::stof(str.substr(start, pos - start));
}

int OBJParserD3D11::ParseInt(const std::string& str, size_t& pos)
{
	while (pos < str.size() && std::isspace(str[pos]))
	{
		pos++;
	}
	size_t start = pos;
	while (pos < str.size() && str[pos] != '/' && !std::isspace(str[pos]))
	{
		pos++;
	}
	return std::stoi(str.substr(start, pos - start));
}

std::string OBJParserD3D11::ParseString(const std::string& str, size_t& pos)
{
	while (pos < str.size() && std::isspace(str[pos]))
	{
		pos++;
	}
	size_t start = pos;
	while (pos < str.size() && !std::isspace(str[pos]) && str[pos] != '\r' && str[pos] != '\n')
	{
		pos++;
	}
	return str.substr(start, pos - start);
}

void OBJParserD3D11::ParsePosition(const std::string& line, ParseData& data)
{
	size_t pos = 0;
	DirectX::XMFLOAT3 position;
	position.x = ParseFloat(line, pos);
	position.y = ParseFloat(line, pos);
	position.z = ParseFloat(line, pos);
	data.positions.push_back(position);
}

void OBJParserD3D11::ParseUV(const std::string& line, ParseData& data)
{
	size_t pos = 0;
	DirectX::XMFLOAT2 uv;
	uv.x = ParseFloat(line, pos);
	uv.y = 1.0f - ParseFloat(line, pos);
	data.texCoords.push_back(uv);
}

void OBJParserD3D11::ParseNormal(const std::string& line, ParseData& data)
{
	size_t pos = 0;
	DirectX::XMFLOAT3 normal;
	normal.x = ParseFloat(line, pos);
	normal.y = ParseFloat(line, pos);
	normal.z = ParseFloat(line, pos);
	data.normals.push_back(normal);
}

void OBJParserD3D11::ParseFace(const std::string& line, ParseData& data)
{
	size_t pos = 0;
	
	for (int i = 0; i < 3; i++)
	{
		while (pos < line.size() && std::isspace(line[pos]))
		{
			pos++;
		}

		size_t start = pos;
		while (pos < line.size() && !std::isspace(line[pos]))
		{
			pos++;
		}
		std::string vertexKey = line.substr(start, pos - start);

		auto it = data.uniqueVertices.find(vertexKey);
		if (it != data.uniqueVertices.end())
		{
			data.indexData.push_back(it->second);
		}
		else
		{
			size_t keyPos = 0;
			int posIndex = ParseInt(vertexKey, keyPos) - 1;
			keyPos++;
			int uvIndex = ParseInt(vertexKey, keyPos) - 1;
			keyPos++;
			int normalIndex = ParseInt(vertexKey, keyPos) - 1;

			DirectX::XMFLOAT3 position = data.positions[posIndex];
			DirectX::XMFLOAT2 uv = data.texCoords[uvIndex];
			DirectX::XMFLOAT3 normal = data.normals[normalIndex];

			uint32_t newIndex = static_cast<uint32_t>(data.vertexData.size() / 8);
			data.vertexData.push_back(position.x);
			data.vertexData.push_back(position.y);
			data.vertexData.push_back(position.z);

			data.vertexData.push_back(uv.x);
			data.vertexData.push_back(uv.y);

			data.vertexData.push_back(normal.x);
			data.vertexData.push_back(normal.y);
			data.vertexData.push_back(normal.z);

			data.uniqueVertices[vertexKey] = newIndex;
			data.indexData.push_back(newIndex);

		}
	}
}

void OBJParserD3D11::FinilzeSubMesh(ID3D11Device* device,ParseData& data)
{
	size_t count = data.indexData.size() - data.currentSubMeshStart;
	if (count == 0)
	{
		return;
	}

	MeshData::SubMeshInfo info;
	info.startIndexValue = data.currentSubMeshStart;
	info.nrOfIndicesInSubMesh = count;

	auto matIt = data.materials.find(data.currentMaterial);

	info.ambientTextureSRV = (matIt != data.materials.end()) 
		? GetOrLoadTexture(device, matIt->second.mapKa) : GetDefaultTexture(device);
	info.diffuseTextureSRV = (matIt != data.materials.end())
		? GetOrLoadTexture(device, matIt->second.mapKd) : GetDefaultTexture(device);
	info.specularTextureSRV = (matIt != data.materials.end())
		? GetOrLoadTexture(device, matIt->second.mapKs) : GetDefaultTexture(device);

	data.subMeshes.push_back(info);
	data.currentSubMeshStart = data.indexData.size();
}

MeshD3D11* OBJParserD3D11::LoadMesh(ID3D11Device* device, const std::string& objFilePath)
{
	auto chaced = loadedMeshes.find(objFilePath);
	if (chaced != loadedMeshes.end())
	{
		return chaced->second;
	}

	std::ifstream file(baseDirectory + objFilePath);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open OBJ-file:" + objFilePath);

	}

	ParseData data;
	std::string line;

	while(std::getline(file,line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		size_t spacePos = line.find(' ');
		if (spacePos == std::string::npos)
		{
			continue;
		}

		std::string id = line.substr(0, spacePos);
		std::string dataPart = line.substr(spacePos + 1);

		if (id == "v")
		{
			ParsePosition(dataPart, data);
		}
		else if (id == "vt")
		{
			ParseUV(dataPart, data);
		}
		else if (id == "vn")
		{
			ParseNormal(dataPart, data);
		}
		else if (id == "f")
		{
			ParseFace(dataPart, data);
		}
		else if (id == "usemtl")
		{
			FinilzeSubMesh(device, data);
			
		}
		else if (id == "mtllib")
		{
			std::string mtlFilePath = baseDirectory + ParseString(dataPart, spacePos);
			try
			{
				ParseMTLFile(mtlFilePath, data);
			}
			catch (const std::exception& e)
			{
				std::runtime_error("Error during reading of MTL-file: ");
			}

		}

	}

	FinilzeSubMesh(device, data);

	MeshData meshData;
	meshData.vertexInfo.sizeOfVertex = sizeof(Vertex);
	meshData.vertexInfo.nrOfVerticesInBuffer = data.vertexData.size() / 8;
	meshData.vertexInfo.vertexData = data.vertexData.data();
	meshData.indexInfo.nrOfIndicesInBuffer = data.indexData.size();
	meshData.indexInfo.indexData = data.indexData.data();
	meshData.subMeshInfo = data.subMeshes;

	MeshD3D11* mesh = new MeshD3D11();
	mesh->Initialize(device, meshData);
	loadedMeshes[objFilePath] = mesh;
	return mesh;

}

void OBJParserD3D11::ParseMTLFile(const std::string& mtlFilePath, ParseData& data)
{
	std::ifstream file(mtlFilePath);
	if (!file.is_open())
	{
		return;
	}

	std::string mtlDir = mtlFilePath.substr(0, mtlFilePath.find_last_of("/\\") + 1);

	std::string line;
	std::string currentMaterial = "";
	ParsedMaterial material;
	bool hasCurrentMaterial = false;

	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		size_t spacePos = line.find(' ');
		if (spacePos == std::string::npos)
		{
			continue;
		}

		std::string id = line.substr(0, spacePos);
		std::string dataPart = line.substr(spacePos + 1);

		if (id == "newmtl")
		{
			if (hasCurrentMaterial)
			{
				data.materials[currentMaterial] = material;
			}

			currentMaterial = dataPart;
			material = ParsedMaterial();
			hasCurrentMaterial = true;
		}
		else if (id == "map_Kd")
		{
			material.mapKd = mtlDir + dataPart;
		}
		else if (id == "map_Ks")
		{
			material.mapKs = mtlDir + dataPart;
		}
		else if (id == "map_Ka")
		{
			material.mapKa = mtlDir + dataPart;
		}
		else if (id == "Ns")
		{
			size_t pos = 0;
			material.shininess = ParseFloat(dataPart, pos);
		}



	}

	if (hasCurrentMaterial)
	{
		data.materials[currentMaterial] = material;
	}
}

ID3D11ShaderResourceView* OBJParserD3D11::GetOrLoadTexture(ID3D11Device* device, const std::string& texturePath)
{
	if (texturePath.empty())
	{
		return GetDefaultTexture(device);
	}

	auto it = loadedTextures.find(texturePath);
	if (it != loadedTextures.end())
	{
		return it->second->GetSRV();
	}

	ShaderResourceTextureD3D11* texture = new ShaderResourceTextureD3D11();
	try
	{
		texture->Initialize(device, texturePath.c_str());
		loadedTextures[texturePath] = texture;
		return texture->GetSRV();
	}
	catch (const std::exception& e)
	{
		delete texture;
		return GetDefaultTexture(device);
	}
}

ID3D11ShaderResourceView* OBJParserD3D11::GetDefaultTexture(ID3D11Device* device)
{
	if (defaultTexture != nullptr)
	{
		return defaultTexture->GetSRV();
	}

	uint8_t whitePixel[4] = { 255, 255, 255, 255 };

	defaultTexture = new ShaderResourceTextureD3D11();
	defaultTexture->Initialize(device, 1, 1, whitePixel);
	return defaultTexture->GetSRV();
}

OBJParserD3D11::~OBJParserD3D11()
{
	for (auto& pair : loadedTextures)
	{
		delete pair.second;
	}
	for (auto& pair : loadedMeshes)
	{
		delete pair.second;
	}
	delete defaultTexture;
}


