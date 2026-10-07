#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udgobj.h"


int main(int argc, char *argv[])
{
	char *exePathBuffer = argv[0];
	
	printf("%s\n", exePathBuffer);
	
	for (int i = 0; i < 1; i++)
	{
		char *lastBackslash = strrchr(exePathBuffer, '\\');
		if (lastBackslash == NULL)
		{
			lastBackslash = strrchr(exePathBuffer, '/');
		}
		if (lastBackslash == NULL)
		{
			free(exePathBuffer);
			return 0;
		}
		*lastBackslash = '\0';
	}
	
	printf("%s\n", exePathBuffer);
	
	
	char concatenatedFilePath[strlen(exePathBuffer) + 256];
	snprintf(concatenatedFilePath, strlen(exePathBuffer) + 256, "%s%s%s", exePathBuffer, "\\resources", "\\Cube.obj");
	free(exePathBuffer);
	
	printf("%s\n", concatenatedFilePath);
	
	UdoErrorCode error;
	int line;
	UdoModel* model = udo_file_read_obj(concatenatedFilePath, &error, &line);
	if (error != UDO_ERROR_NONE)
	{
		printf("Error %s at line %d\n", udo_error_code_to_string(error), line);
		return 0;
	}

	/*
	for (int i = 0; i < model->geometricVerticesCount; i++)
	{
		UdoVector4 vertex = model->geometricVertices[i];
		printf("Vertex at count %d: %f %f %f %f\n", i + 1, vertex.x, vertex.y, vertex.z, vertex.w);
	}
	
	printf("\n");
	
	for (int i = 0; i < model->faceElementsCount; i++)
	{
		UdoFaceDefinition face = model->faceElements[i];
		printf("Face %d with %zu vertices\n", i + 1, face.verticesCount);
		
		for (int j = 0; j < face.verticesCount; j++)
		{
			UdoVertexReference vertex = face.vertices[j];
			printf("Vertices %d, %d, %d\n", vertex.geometricVertexIndex, vertex.textureVertexIndex, vertex.normalVertexIndex);
		}
		
		printf("\n");
	}*/
	
	
	for (int i = 0; i < udo_model_get_triangle_count(*model); i++)
	{
		UdoVertexDefinition* triangle = udo_model_get_triangle(*model, i);
		if (triangle == NULL)
			continue;
		
		printf("Triangle %d:\n", i + 1);
		for (int j = 0; j < 3; j++)
		{
			UdoVector4 vertex = triangle[j].geometricVertex;
			printf("%f %f %f\n", vertex.x, vertex.y, vertex.z);
		}
		printf("\n");
		
		free(triangle);
	}
	
	udo_model_free(model);
	
    return 0;
}