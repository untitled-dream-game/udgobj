#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "udgobj.h"



// --------------------------------------------------
// Parsing
// --------------------------------------------------

// --------------------------------------------------
// Type Definitions
// --------------------------------------------------
typedef enum 
{
	PARSE_MODE_UNSUPPORTED,
	
	PARSE_MODE_GEOMETRIC_VERTEX,
	PARSE_MODE_FACE_ELEMENT,
	
	PARSE_MODE_COUNT,
} ObjParseMode;


// --------------------------------------------------
// Helper Functions
// --------------------------------------------------
static bool str_is_whitespace(char *str)
{
	int i = 0;
	while (true)
	{
		if (str[i] == '\0')
			break;
		
		if (!isspace((unsigned char)str[i]))
		{
			return false;
		}
		i++;
	}
	return true;
}

static ObjParseMode obj_get_parse_mode_from_keyword(char* keyword)
{
	if (strcmp(keyword, "v") == 0)
	{
		return PARSE_MODE_GEOMETRIC_VERTEX;
	}
	if (strcmp(keyword, "f") == 0)
	{
		return PARSE_MODE_FACE_ELEMENT;
	}
	return PARSE_MODE_UNSUPPORTED;
}


// --------------------------------------------------
// Implementation
// --------------------------------------------------
static void fill_geometric_vertices_data(FILE* file, UdoModel* model, UdoErrorCode *errorCode, int *errorLine)
{
	if (model->geometricVerticesCount == 0)
	{
		*errorCode = UDO_ERROR_NONE; // Don't process geometric vertices if there's 0 of them, for safety.
		*errorLine = -1;
		return;
	}
	
	int bufferLength = 255;
	char buffer[bufferLength];
	char bufferCopy[bufferLength];
	
	int currentLineCount = 0;
	while (true)
	{
		if (fgets(buffer, bufferLength, file) == NULL)
			break;
		
		bool isOnlyWhitespace = str_is_whitespace(buffer);
		if (isOnlyWhitespace)
			continue;
		
		strcpy(bufferCopy, buffer);
		char *itemToken = strtok(bufferCopy, " \t\r\n");
		
		ObjParseMode parseMode = obj_get_parse_mode_from_keyword(itemToken);
		if (parseMode != PARSE_MODE_GEOMETRIC_VERTEX)
			continue;
		
		itemToken = strtok(NULL, " \t\r\n");
		int itemCount = 0;
		while (itemToken != NULL) 
		{
			char *errorPointer;
			double vertexCoordinate = strtod(itemToken, &errorPointer);
			if (errorPointer == itemToken)
			{
				*errorCode = UDO_ERROR_FILE_MALFORMED_V; // Non-real number.
				*errorLine = currentLineCount;
				return;
			}
			
			switch (itemCount)
			{
				case 0:
					model->geometricVertices[currentLineCount].x = vertexCoordinate;
					break;
				case 1:
					model->geometricVertices[currentLineCount].y = vertexCoordinate;
					break;
				case 2:
					model->geometricVertices[currentLineCount].z = vertexCoordinate;
					break;
				case 3:
					model->geometricVertices[currentLineCount].w = vertexCoordinate;
					break;
				default:
					break;
			}
			
			itemCount++;
			itemToken = strtok(NULL, " \t\r\n");
		}
		if (itemCount < 3)
		{
			*errorCode = UDO_ERROR_FILE_MALFORMED_V; // Geometric vertex has <3 coordinates.
			*errorLine = currentLineCount;
			return;
		}
		if (itemCount == 3)
		{
			model->geometricVertices[currentLineCount].w = 1.0;
		}
		// Coordinates after the fourth one get ignored, should this be reported?
		
		currentLineCount++;
		if (currentLineCount >= model->geometricVerticesCount)
			break;
	}
	
	*errorCode = UDO_ERROR_NONE;
	*errorLine = -1;
}

typedef enum 
{
	FACE_V,
	FACE_V_VT,
	FACE_V_VT_VN,
	FACE_V_VN,
	
	FACE_INVALID
} FaceElementFormat;

static FaceElementFormat get_face_element_format(char* item)
{
	char *backslashPosition = strchr(item, '/');
	if (backslashPosition == NULL)
		return FACE_V;

	char *previousBackslashPosition = backslashPosition;
	backslashPosition++;
	backslashPosition = strchr(backslashPosition, '/');
	if (backslashPosition == NULL)
		return FACE_V_VT;

	bool hasNoTextureVertices = previousBackslashPosition[1] == '/';
    
	previousBackslashPosition = backslashPosition;
	backslashPosition++;
	backslashPosition = strchr(backslashPosition, '/');
	if (backslashPosition != NULL)
		return FACE_INVALID; // Trying to specify more than 3 vertex indices.
	
	return hasNoTextureVertices ? FACE_V_VN : FACE_V_VT_VN;
}

static UdoErrorCode fill_individual_face_element(char* line, int lineLength, UdoFaceDefinition* faceElement)
{
	char lineCopy[lineLength];
	strcpy(lineCopy, line);
	
	int itemsCount = 0;
	char *itemToken = strtok(lineCopy, " \t\r\n");
	while (itemToken != NULL)
	{
		itemsCount++;
		itemToken = strtok(NULL, " \t\r\n");
	}
	
	if (itemsCount < 3)
		return UDO_ERROR_FILE_MALFORMED_F;
	
	faceElement->verticesCount = itemsCount;
	faceElement->vertices = malloc(itemsCount * sizeof(UdoVertexReference));
	
	
	char itemTokenCopy[lineLength];
	strcpy(lineCopy, line);
	itemToken = strtok(lineCopy, " \t\r\n");
	strcpy(itemTokenCopy, itemToken);
	
	FaceElementFormat intendedFormat = get_face_element_format(itemTokenCopy);
	if (intendedFormat == FACE_INVALID)
		return UDO_ERROR_FILE_MALFORMED_F;	
	
	int intendedElementCount;
	switch (intendedFormat)
	{
		case FACE_V:
			intendedElementCount = 1;
			break;
		case FACE_V_VT:
			intendedElementCount = 2;
			break;
		case FACE_V_VT_VN:
		case FACE_V_VN:
			intendedElementCount = 3;
			break;
		default:
			intendedElementCount = 0;
			break;
	}
	
	
	char *errorPointer;
	itemsCount = 0;

	strcpy(lineCopy, line);
	itemToken = strtok(lineCopy, " \t\r\n");
	while (itemToken != NULL) 
	{
		strcpy(itemTokenCopy, itemToken);
		
		faceElement->vertices[itemsCount].geometricVertexIndex = 0;
		faceElement->vertices[itemsCount].textureVertexIndex = 0;
		faceElement->vertices[itemsCount].normalVertexIndex = 0;	
		
		FaceElementFormat thisFormat = get_face_element_format(itemTokenCopy);
		if (intendedFormat != thisFormat)
			return UDO_ERROR_FILE_MALFORMED_F;		
		
		char *elementToken = itemTokenCopy;
		for (int i = 0; i < intendedElementCount; i++)
		{
			if ((i == 1) && (thisFormat == FACE_V_VN))
			{
				elementToken = strchr(elementToken, '/');
				elementToken++;
				continue;
			}
			
			char elementTokenCopy[strlen(elementToken)];
			strcpy(elementTokenCopy, elementToken);
		
			long vertexElement = strtol(elementTokenCopy, &errorPointer, 10);
			if (errorPointer == elementTokenCopy)
				return UDO_ERROR_FILE_MALFORMED_F;
			
			switch (i)
			{
				case 0:
					faceElement->vertices[itemsCount].geometricVertexIndex = (int)vertexElement;
					break;
				case 1:
					faceElement->vertices[itemsCount].textureVertexIndex = (int)vertexElement;
					break;
				case 2:
					faceElement->vertices[itemsCount].normalVertexIndex = (int)vertexElement;
					break;
			}
			
			elementToken = strchr(elementToken, '/');
			if (i != (intendedElementCount - 1))
			{
				elementToken++;
			}
		}

		itemsCount++;
		itemToken = strtok(NULL, " \t\r\n");
	}

	return UDO_ERROR_NONE;
}


static void fill_face_elements_data(FILE* file, UdoModel* model, UdoErrorCode *errorCode, int *errorLine)
{
	if (model->faceElementsCount == 0)
	{
		*errorCode = UDO_ERROR_NONE;
		*errorLine = -1;
		return;
	}

	int bufferLength = 255;
	char buffer[bufferLength];
	char bufferCopy[bufferLength];
	
	int currentLineCount = 0;
	while (true)
	{
		if (fgets(buffer, bufferLength, file) == NULL)
			break;
		
		bool isOnlyWhitespace = str_is_whitespace(buffer);
		if (isOnlyWhitespace)
			continue;
		
		strcpy(bufferCopy, buffer);
		char *itemToken;
		
		itemToken = strtok(bufferCopy, " \t\r\n");
		
		ObjParseMode parseMode = obj_get_parse_mode_from_keyword(itemToken);
		if (parseMode != PARSE_MODE_FACE_ELEMENT)
			continue;

		strcpy(bufferCopy, buffer);
		char *bufferWithoutKeyword = bufferCopy + 2;
		UdoErrorCode errorCodeIndividualFaceElement = fill_individual_face_element(bufferWithoutKeyword, bufferLength, &model->faceElements[currentLineCount]);
		if (errorCodeIndividualFaceElement != UDO_ERROR_NONE)
		{
			*errorCode = UDO_ERROR_FILE_MALFORMED_V;
			*errorLine = currentLineCount;
			return;
		}
		
		currentLineCount++;
		if (currentLineCount >= model->faceElementsCount)
			break;
	}
	
	*errorCode = UDO_ERROR_NONE;
	*errorLine = -1;
}


UdoModel* udo_file_read_obj(const char *filepath, UdoErrorCode *errorCode, int *errorLine)
{
	// How its implemented:
	// First pass reads the file and measures positions + lengths of each data type (accounting for whitespace).
	// Additional passes are done for each data type that read specific portions and fills appropriate data.
	
	FILE* file = fopen(filepath, "r");
	
	if (file == NULL)
	{
		if (errorCode != NULL)
			*errorCode = UDO_ERROR_COULDNT_OPEN_FILE;
		if (errorLine != NULL)
			*errorLine = -1;
		return NULL;
	}
	
	UdoModel *model = NULL;
	bool shouldFreeModel = true;
	
	int bufferLength = 255;
	char buffer[bufferLength];
	char bufferCopy[bufferLength];
	
	
	int currentLineCount = -1; // Incremented at the start, so start at -1 to always have 0-indexed lines.
	int currentNonWhitespaceLineCount = 0;
	long currentLinePosition = ftell(file);

	long* dataPositionsChars = calloc(PARSE_MODE_COUNT, sizeof(long));
	long* dataPositionsLines = calloc(PARSE_MODE_COUNT, sizeof(long));
	int* dataLengths = calloc(PARSE_MODE_COUNT, sizeof(int));
	if (dataPositionsChars == NULL || dataLengths == NULL)
	{
		if (errorCode != NULL)
			*errorCode = UDO_ERROR_OUT_OF_MEMORY;
		if (errorLine != NULL)
			*errorLine = -1;
		goto clean_up;
	}
	
	while (fgets(buffer, bufferLength, file)) 
	{
		currentLineCount++;
		
		bool isOnlyWhitespace = str_is_whitespace(buffer);
		if (isOnlyWhitespace)
			continue;
		
		
		strcpy(bufferCopy, buffer);
		char *bufferToken = strtok(bufferCopy, " \t\r\n");
		ObjParseMode parseMode = obj_get_parse_mode_from_keyword(bufferToken);
		
		if (parseMode == PARSE_MODE_UNSUPPORTED)
			continue;
		
		bool isFirstInstanceOfData = dataLengths[parseMode] == 0;
		if (isFirstInstanceOfData)
		{
			dataPositionsChars[parseMode] = currentLinePosition;
			dataPositionsLines[parseMode] = currentLineCount;
			dataLengths[parseMode] = 0;
		}
		dataLengths[parseMode]++;
		
		currentNonWhitespaceLineCount++;
		currentLinePosition = ftell(file); // Call ftell right before calling fgets to fetch the correct offset.
	}
	
	
	model = malloc(sizeof(UdoModel));
	
	if (model == NULL)
	{
		if (errorCode != NULL)
			*errorCode = UDO_ERROR_OUT_OF_MEMORY;
		if (errorLine != NULL)
			*errorLine = -1;
		goto clean_up;
	}

	model->geometricVerticesCount = dataLengths[PARSE_MODE_GEOMETRIC_VERTEX];
	model->geometricVertices = malloc(model->geometricVerticesCount * sizeof(UdoVector4));
	if (model->geometricVertices == NULL)
	{
		if (errorCode != NULL)
			*errorCode = UDO_ERROR_OUT_OF_MEMORY;
		if (errorLine != NULL)
			*errorLine = -1;
		goto clean_up;
	}
	
	model->faceElementsCount = dataLengths[PARSE_MODE_FACE_ELEMENT];
	model->faceElements = malloc(model->faceElementsCount * sizeof(UdoFaceDefinition));
	if (model->faceElements == NULL)
	{
		if (errorCode != NULL)
			*errorCode = UDO_ERROR_OUT_OF_MEMORY;
		if (errorLine != NULL)
			*errorLine = -1;
		goto clean_up;
	}
	
	UdoErrorCode errorCodeFilling;
	int errorLineFilling;
	
	fseek(file, dataPositionsChars[PARSE_MODE_GEOMETRIC_VERTEX], SEEK_SET);
	fill_geometric_vertices_data(file, model, &errorCodeFilling, &errorLineFilling);
	if (errorCodeFilling != UDO_ERROR_NONE)
	{
		if (errorCode != NULL)
			*errorCode = errorCodeFilling;
		if (errorLine != NULL)
			*errorLine = dataPositionsLines[PARSE_MODE_GEOMETRIC_VERTEX] + errorLineFilling + 1;
		goto clean_up;
	}
	
	fseek(file, dataPositionsChars[PARSE_MODE_FACE_ELEMENT], SEEK_SET);
	fill_face_elements_data(file, model, &errorCodeFilling, &errorLineFilling);
	if (errorCodeFilling != UDO_ERROR_NONE)
	{
		if (errorCode != NULL)
			*errorCode = errorCodeFilling;
		if (errorLine != NULL)
			*errorLine = dataPositionsLines[PARSE_MODE_FACE_ELEMENT] + errorLineFilling + 1;
		goto clean_up;
	}
	
	shouldFreeModel = false;
	if (errorCode != NULL)
		*errorCode = UDO_ERROR_NONE;
	if (errorLine != NULL)
		*errorLine = -1;
	
	clean_up:
	
	fclose(file);
	free(dataPositionsChars); free(dataLengths);
	if (shouldFreeModel)
	{
		udo_model_free(model);	
		model = NULL;
	}
	
	return model;
}



// --------------------------------------------------
// .obj Models
// --------------------------------------------------

size_t udo_model_get_triangle_count(UdoModel model)
{
	int triangleCountTotal = 0;
	for (int i = 0; i < model.faceElementsCount; i++)
	{
		UdoFaceDefinition face = model.faceElements[i];
		int triangleCount = face.verticesCount - 2; // Triangle fan order: 1-2-3, 1-3-4, 1-4-5...
		triangleCountTotal += triangleCount;
	}
	return triangleCountTotal;
}

UdoVertexDefinition* udo_model_get_triangle(UdoModel model, int triangleIndex)
{
	if (triangleIndex < 0 || triangleIndex >= udo_model_get_triangle_count(model))
		return NULL;
	
	int triangleCountTotal = 0;
	for (int i = 0; i < model.faceElementsCount; i++)
	{
		UdoFaceDefinition face = model.faceElements[i];
		int triangleCount = face.verticesCount - 2; // Triangle fan order: 1-2-3, 1-3-4, 1-4-5...
		for (int j = 0; j < triangleCount; j++)
		{
			if (triangleIndex != triangleCountTotal)
			{
				triangleCountTotal++;
				continue;
			}
			
			UdoVertexDefinition* triangle = malloc(3 * sizeof(UdoVertexDefinition));
			
			UdoVertexReference vertexRefs[3];
			vertexRefs[0] = face.vertices[0];
			vertexRefs[1] = face.vertices[j + 1];
			vertexRefs[2] = face.vertices[j + 2];
			
			for (int k = 0; k < 3; k++)
			{
				UdoVertexReference vertexRef = vertexRefs[k];
				int geometricVertexRelativeIndex = vertexRef.geometricVertexIndex;
				bool isIndexInvalid = false;
				isIndexInvalid |= geometricVertexRelativeIndex == 0;
				isIndexInvalid |= (abs(geometricVertexRelativeIndex) - 1) >= model.geometricVerticesCount;
				if (isIndexInvalid)
				{
					free(triangle);
					return NULL;
				}
				
				int geometricVertexAbsoluteVertex;
				if (geometricVertexRelativeIndex < 0)
				{
					geometricVertexAbsoluteVertex = model.geometricVerticesCount + geometricVertexRelativeIndex;
				}
				else
				{
					geometricVertexAbsoluteVertex = geometricVertexRelativeIndex - 1;
				}
				
				triangle[k].geometricVertex = model.geometricVertices[geometricVertexAbsoluteVertex];
			}
			
			return triangle;
		}
	}
	
	return NULL;
}

void udo_model_free(UdoModel* model)
{
	if (model == NULL)
		return;
	
	if (model->geometricVertices != NULL)
	{
		free(model->geometricVertices);
	}
	
	if (model->faceElements != NULL)
	{
		free(model->faceElements);
	}
	
	free(model);
}



// --------------------------------------------------
// Miscellaneous
// --------------------------------------------------

char* udo_error_code_to_string(UdoErrorCode errorCode)
{
	switch (errorCode)
	{
		case UDO_ERROR_NONE:
			return "No error";
		case UDO_ERROR_COULDNT_OPEN_FILE:
			return "Couldn't open file";
		case UDO_ERROR_OUT_OF_MEMORY:
			return "Ran out of memory";
		case UDO_ERROR_FILE_MALFORMED_V:
			return "Data of type 'v' is malformed";
		case UDO_ERROR_FILE_MALFORMED_F:
			return "Data of type 'f' is malformed";
		default:
			return "";
	}
}