
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