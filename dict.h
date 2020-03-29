/**
 * @file dict.h
 * @brief Interfaccia per la struttura dati dict.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#ifndef DICT_H_
#define DICT_H_

#include <utils.h>
#include <limits.h>

/**
 * @struct Entry_t
 */
typedef struct {

	long long int key;      /**< Chiave dell'entry, rappresenta l'id del client. */

	struct value_t {        /**< Valore associato alla chiave. */
		int miglior_stima;
		int count_server;
	} value;

} Entry_t;

/**
 * @struct Dict_t
 */
typedef struct {
    
	int len;        /**< Lunghezza del dizionario. */
    Entry_t* entry; /**< Array, allocato dinamicamente, di Entry_t. */

} Dict_t;

#define foreach(dict, key, value) 			\
	for (int i = 0; i < dict->len; i++) 	\
		for (key = dict->entry[i].key, value = dict->entry[i].value; key == dict->entry[i].key; key++)

/**
 * @function initDict
 * @brief Restituisce un puntatore ad un oggetto Dict_t 
 *        allocato dinamicamente, correttamente inizializzato.
 */
Dict_t* initDict();

/**
 * @function deleteDict
 * @brief Distrugge l'oggetto @dict, passato come parametro,
 *        liberando quindi tutta la memoria occupata.
 */
void deleteDict(Dict_t* dict);

/**
 * @function add
 * @brief Aggiunge una nuova entry <key, value> in @dict, se @key non è presente in @dict,
 *        altrimenti aggiorna con @value il valore associato a @key in @dict.
 */
void add(Dict_t* dict, long long int key, struct value_t value);

/**
 * @function get_value
 * @brief Se la chiave @key è presente in dict mi restituisce il value associato,
 *        altrimenti restituisce un value inizializzato come {INT_MAX, 0}.
 */
struct value_t get_value(Dict_t* dict, long long int key);

#endif // DICT_H_
