#include "obj.hh"

#include <cstdio>
#include <vector>
#include <string>
#include <cstring>

using namespace std;


OBJ::OBJ()
{
}


/*
 * OBJ::Load() - Copied with slight modifications from
 * http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/
 */
bool OBJ::Load( const string &filepath )
{
	vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	vector<glm::vec3> temp_vertices;
	vector<glm::vec2> temp_uvs;
	vector<glm::vec3> temp_normals;

	FILE *file;

#ifdef _WIN32
	fopen_s( &file, filepath.c_str(), "r" );
#else
	file= fopen( filepath.c_str(), "r" );
#endif

	if( file == NULL )
	{
		cout << "Couldn't open file \"" << filepath.c_str() << "\"!\n";
		return false;
	}

	while( true )
	{
		char lineHeader[128];
		#ifdef _WIN32
			int res = fscanf_s( file, "%s", lineHeader, sizeof( lineHeader ) );
		#else
			int res = fscanf( file, "%s", lineHeader );
		#endif

		if( res == EOF )
		{
			break;
		}

		if( strcmp( lineHeader, "v" ) == 0 )
		{
			glm::vec3 vertex;
			#ifdef _WIN32
				fscanf_s( file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z, sizeof( float ) );
			#else
				fscanf( file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
			#endif
			temp_vertices.push_back( vertex );
		}

		else if ( strcmp( lineHeader, "vt" ) == 0 )
		{
			glm::vec2 uv;
			#ifdef _WIN32
				fscanf_s( file, "%f %f\n", &uv.x, &uv.y, sizeof( float ) );
			#else
				fscanf( file, "%f %f\n", &uv.x, &uv.y );
			#endif
			temp_uvs.push_back( uv );
		}

		else if( strcmp( lineHeader, "vn" ) == 0 )
		{
			glm::vec3 normal;
			#ifdef _WIN32
				fscanf_s( file, "%f %f %f\n", &normal.x, &normal.y, &normal.z, sizeof( float ) );
			#else
				fscanf( file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
			#endif
			temp_normals.push_back( normal );
		}

		else if( strcmp( lineHeader, "f" ) == 0 )
		{
			string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];

			#ifdef _WIN32
				int matches = fscanf_s(
					file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
					&vertexIndex[0], &uvIndex[0], &normalIndex[0],
					&vertexIndex[1], &uvIndex[1], &normalIndex[1],
					&vertexIndex[2], &uvIndex[2], &normalIndex[2],
					sizeof( unsigned int )
				);
			#else
				int matches = fscanf(
					file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
					&vertexIndex[0], &uvIndex[0], &normalIndex[0],
					&vertexIndex[1], &uvIndex[1], &normalIndex[1],
					&vertexIndex[2], &uvIndex[2], &normalIndex[2]
				);
			#endif

			if( matches != 9)
			{
				cout << "File can't be read by our simple parser : ( Try exporting with other options" << endl;
				return false;
			}

			vertexIndices.push_back( vertexIndex[0] );
			vertexIndices.push_back( vertexIndex[1] );
			vertexIndices.push_back( vertexIndex[2] );
			uvIndices.push_back( uvIndex[0] );
			uvIndices.push_back( uvIndex[1] );
			uvIndices.push_back( uvIndex[2] );
			normalIndices.push_back( normalIndex[0] );
			normalIndices.push_back( normalIndex[1] );
			normalIndices.push_back( normalIndex[2] );
		}
	}

	for( auto&ind : vertexIndices )
	{
		vertices.push_back( temp_vertices[ind-1] );
	}

	for( auto&ind : uvIndices )
	{
		uvs.push_back( temp_uvs[ind-1] );
	}

	for( auto&ind : normalIndices )
	{
		normals.push_back( temp_normals[ind-1] );
	}

	return true;
}

