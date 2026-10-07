#ifndef UDGOBJ_H
#define UDGOBJ_H



typedef enum
{
	UDO_ERROR_NONE,
	
	UDO_ERROR_COULDNT_OPEN_FILE,
	UDO_ERROR_OUT_OF_MEMORY,
	
	UDO_ERROR_FILE_MALFORMED_V,
	UDO_ERROR_FILE_MALFORMED_F,
} UdoErrorCode;



typedef struct UdoVector4
{
	double x;
	double y;
	double z;
	double w;
} UdoVector4;

typedef struct UdoVertexReference
{
	int geometricVertexIndex;
	int textureVertexIndex;
	int normalVertexIndex;
} UdoVertexReference;

typedef struct UdoVertexDefinition
{
	UdoVector4 geometricVertex;
	// texture vertex
	// normal vertex
} UdoVertexDefinition;

typedef struct UdoFaceDefinition
{
	size_t verticesCount;
	UdoVertexReference* vertices;
} UdoFaceDefinition;

typedef struct UdoModel
{
    size_t geometricVerticesCount;
	UdoVector4 *geometricVertices;
	
    size_t faceElementsCount;
	UdoFaceDefinition *faceElements;
} UdoModel;



UdoModel* udo_file_read_obj(const char *filepath, UdoErrorCode *errorCode, int *errorLine);

size_t udo_model_get_triangle_count(UdoModel model);
UdoVertexDefinition* udo_model_get_triangle(UdoModel model, int triangleIndex);
void udo_model_free(UdoModel* model);

char* udo_error_code_to_string(UdoErrorCode errorCode);

#endif //UDGOBJ_H