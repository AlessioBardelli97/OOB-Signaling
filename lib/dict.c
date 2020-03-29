/**
 * @file dict.c
 * @brief Implementazione della struttura dati dict.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#include <dict.h>

Dict_t* initDict() {
    
	Dict_t* result = NULL;
	REALLOC(result, sizeof(Dict_t), "initDict: realloc", return NULL)

	result->len = 0;
	result->entry = NULL;

	return result;
}

void deleteDict(Dict_t* dict) {
    
	if (!dict)
		return;

	if (dict->entry)
		free(dict->entry);

	free(dict);
}

void add(Dict_t* dict, long long int key, struct value_t value) {
	
	int i;

	for (i = 0; i < dict->len; i++)
		if (dict->entry[i].key == key)
			break;
	
	if (i == dict->len)	{

		int newsize = (dict->len+1) * sizeof(Entry_t);
		REALLOC(dict->entry, newsize, "addDict: realloc", return)

		dict->entry[i].key = key;
		dict->len += 1;
	}

	dict->entry[i].value = value;
}

struct value_t get_value(Dict_t* dict, long long int key) {

	struct value_t result = {INT_MAX, 0};

    for (int i = 0; i < dict->len; i++)
		if (dict->entry[i].key == key)
			return (dict->entry[i].value);

	return result;
}
