//#include "util.h"

struct Token {
	u8* value;
	int size;
	union {
		int int_v;
		float float_v;
	};
};

internal bool str_equal(const char* str1, int str1_size, const char* str2, int str2_size)
{
	if (str1_size == str2_size) {
		for (int i = 0; i < str1_size; ++i) {
			if (str1[i] != str2[i])
				return false;
		}
		return true;
	}
	return false;
}
/*
void load_model(Model3D* model, u8* filename)
{
	s64 fsize = 0;
	u8* m = read_entire_file(filename, &fsize);
	if (m == 0) {
		error_fatal("Could not find file ", (char*)filename);
	}

	int num_vertices = 0;
	u8* ptr = m;
	Token* tokens = (Token*)malloc(fsize * sizeof(Token));
	int num_tokens = 0;
	while (*ptr != 0) {
		while (is_white_space(*ptr)) {
			ptr++;
		}

		if (is_letter(*ptr)) {
			tokens[num_tokens].value = ptr;
			int size = 0;
			while (is_letter(*ptr) || is_number(*ptr)) {
				ptr++;
				size++;
			}
			tokens[num_tokens].size = size;
			num_tokens++;
		}
		if (*ptr == '-' && !is_number(ptr[1])) {
			ptr++;
			continue;
		}

		if (is_number(*ptr) || *ptr == '-') {
			bool negative = false;
			int size = 0;
			tokens[num_tokens].value = ptr;
			if (*ptr == '-' && is_number(ptr[1])) {
				negative = true;
				ptr++;
				size++;
			}
			bool is_float = false;
			while (is_number(*ptr)) {
				size++;
				ptr++;
				if (*ptr == '.' && is_float == false) {
					is_float = true;
					ptr++;
					size++;
				}
			}
			tokens[num_tokens].size = size;
			const char buffer[128] = { 0 };
			memcpy((void*)buffer, tokens[num_tokens].value, size);
			if (is_float) {
				tokens[num_tokens].float_v = atof(buffer);
			}
			else {
				tokens[num_tokens].int_v = atoi(buffer);
			}
			num_tokens++;
		}

		ptr++;
	}

	int index = 0;


	for (int i = 0; i < num_tokens; ++i) {
		if (str_equal((const char*)tokens[i].value, tokens[i].size, "triangles", sizeof "triangles" - 1)) {
			num_vertices = tokens[i + 1].int_v;
			model->vertices = (Vertex3D*)malloc(sizeof(Vertex3D) * num_vertices * 3);
		}

		if (str_equal((const char*)tokens[i].value, tokens[i].size, "v0", sizeof "v0" - 1) ||
			str_equal((const char*)tokens[i].value, tokens[i].size, "v1", sizeof "v1" - 1) ||
			str_equal((const char*)tokens[i].value, tokens[i].size, "v2", sizeof "v2" - 1))
		{
			(model->vertices + index)->pos[0] = tokens[i + 1].float_v;
			(model->vertices + index)->pos[1] = tokens[i + 2].float_v;
			(model->vertices + index)->pos[2] = tokens[i + 3].float_v;

			(model->vertices + index)->normal[0] = tokens[i + 4].float_v;
			(model->vertices + index)->normal[1] = tokens[i + 5].float_v;
			(model->vertices + index)->normal[2] = tokens[i + 6].float_v;
			index++;
		}
	}
	model->num_vertices = num_vertices * 3;
}
*/

/*
struct Vertex3D {
float pos[3];
float normal[3];
float tex[2];
};
*/
float parse_float(u8** at) {
	bool floating_point = false;
	int i = 0;
	int length = 0;
	do {
		++i;
		length++;
		if ((*at)[i] == '.' && !floating_point) {
			floating_point = true;
			++i;
			length++;
		}
	} while ((*at)[i] && (is_number((*at)[i])));
	char buffer[128] = { 0 };
	memcpy(buffer, (void*)*at, length);
	float result = atof(buffer);
	*at += length;
	return result;
}

int parse_int(u8** at)
{
	int length = 0;
	while (is_number((*at)[length])) {
		length++;
	}
	char buffer[128] = { 0 };
	memcpy(buffer, (void*)*at, length);
	*at += length;
	return atoi(buffer);
}

void goto_next_line(u8** at) {
	while (**at != '\n') (*at)++;
}

void eat_whitespace(u8** at)
{
	while (is_white_space(**at)) {
		(*at)++;
	}
}

void parse_normal(u8** at, Vertex3D* vertex_data, int index)
{
	float x, y, z;

	eat_whitespace(at);
	x = parse_float(at);
	eat_whitespace(at);
	y = parse_float(at);
	eat_whitespace(at);
	z = parse_float(at);
	goto_next_line;

	(vertex_data + index)->normal[0] = x;
	(vertex_data + index)->normal[1] = y;
	(vertex_data + index)->normal[2] = z;
}

void parse_vertex(u8** at, Vertex3D* vertex_data, int index)
{
	float x, y, z;

	eat_whitespace(at);
	x = parse_float(at);
	eat_whitespace(at);
	y = parse_float(at);
	eat_whitespace(at);
	z = parse_float(at);
	goto_next_line;

	(vertex_data + index)->pos[0] = x;
	(vertex_data + index)->pos[1] = y;
	(vertex_data + index)->pos[2] = z;
}

void parse_texture_coord(u8** at, Vertex3D* vertex_data, int index)
{
	float x, y;

	eat_whitespace(at);
	x = parse_float(at);
	eat_whitespace(at);
	y = parse_float(at);
	goto_next_line(at);

	(vertex_data + index)->tex[0] = x;
	(vertex_data + index)->tex[1] = y;
}

struct IndexData {
	int vertex;
	int texcoord;
	int normal;
};

void parse_triangle(u8** at, IndexData* index_data, int index) {
	int x, y, z;
	int vindex = 0, tindex, nindex;
	eat_whitespace(at);
	vindex = parse_int(at); (*at)++;
	tindex = parse_int(at); (*at)++;
	nindex = parse_int(at); (*at)++;

	(index_data + index)->vertex = vindex;
	(index_data + index)->texcoord = tindex;
	(index_data + index)->normal = nindex;

	eat_whitespace(at);


}

void load_objfile(char* filename) 
{
	s64 size = 0;
	u8* data = read_entire_file((u8*)filename, &size);
	u8* start_data = data;
	if (!data) {
		char buffer[512] = { 0 };
		sprintf(buffer, "could not read file %s\n", filename);
		error_fatal("File IO error.\n", buffer);
	}

	Vertex3D* vertex_data = (Vertex3D*)malloc(size * sizeof(Vertex3D));
	IndexData* index_data = (IndexData*)malloc(size * sizeof(IndexData));

	int position_index = 0;
	int texcoord_index = 0;
	int normal_index = 0;
	int faces_index = 0;

	while (data != 0)
	{
		//getline(objFile, line);
		if (data[0] == 'v' && data[1] == ' ') {
			data++;
			parse_vertex(&data, vertex_data, position_index);
			position_index++;
		} else if (data[0] == 'v' && data[1] == 't') {
			data += 2;
			parse_texture_coord(&data, vertex_data, texcoord_index);
			texcoord_index++;
		} else if (data[0] == 'v' && data[1] == 'n') {
			data += 2;
			parse_normal(&data, vertex_data, normal_index);
			normal_index++;
		} else if (data[0] == 'f' && data[1] == ' ') {
			data++;
			parse_triangle(&data, index_data, faces_index);
			faces_index++;
		} else {
			while (*data != '\n') {
				if (*data == 0) break;
				data++;
			}
			if (*data == 0) break;
			data++;
		}
	}
	int x = 0;
}