#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dictionary.h"

bool is_upper(char c) {
  return (c >= 65 && c <= 90);
}

bool is_lower(char c) {
  return (c >= 97 && c <= 122);
}

bool is_apostraphe(char c) {
  return c == 39;
}

bool is_number(char c) {
  return (c >= 48 && c <= 57);
}

/*
  Accepts upper case, lower case or apostraphe's
*/
bool is_valid_char(char c) {
  return is_lower(c) || is_upper(c) || is_apostraphe(c) || is_number(c);
}

/*
  Convert a word to lower case and strip trailing punctuation.
*/
char* prep_word(const char* word) {
  char* cleaned = (char *)calloc(1, strlen(word)+1);

  for (int i = 0; i < strlen(word); i++) {
    if (is_upper(word[i]))
      cleaned[i] = word[i] + 32;
    //Already lower case or a single quote for possessive
    else
      cleaned[i] = word[i];
  }

  // Strip trailing bad chars
  for (int i = strlen(word)-1; i >= 0; i--) {
    if (!(is_valid_char(word[i]))) {
      cleaned[i] = '\0';
    } else {
      break;
    }
  }

  return cleaned;
}

bool all_numbers(const char* word) {
  for (int i = 0; i < strlen(word); i++) {
    if (!is_number(word[i]))
      return false;
  }
  return true;
}

bool all_punctuation(const char* word) {
  for (int i = 0; i < strlen(word); i++) {
    if (is_valid_char(word[i]))
      return false;
  }
  return true;
}

/**
 * Returns true if word is in dictionary else false.
 */
/**
 * Inputs:
 *  word:       A word to check the spelling of.
 *  hashtable:  The hash table used to determine spelling
 *
 * Returns:
 *  bool:       A boolean value indicating if the word was correctly spelled.
 *
 * Modifies:
 *
 * Example:
 *  bool correct  = check_word(word, hashtable);
 **/
bool check_word(const char* word, hashmap_t hashtable[])
{
  if (word == NULL || strlen(word) > LENGTH || strlen(word) < 1)
    return false;

  char* cleaned = prep_word(word);

  int bucket = hash_function(cleaned);
  node* cursor;

  //AFL- If a word hashes to a nonvalid bucket, return false
  if (bucket >= 0 && bucket < HASH_SIZE) {
    cursor = hashtable[bucket];
  }
  else {
    free(cleaned);
    return false;
  }

  bool ret = false;
  while (cursor) {
    if (strcmp(cursor->word, cleaned) == 0) {
      ret = true;
      break;
    }

    cursor = cursor->next;
  }

  // Check for all numbers
  if (!ret && all_numbers(cleaned))
    ret = true;

  // Handle " ... " or " - " or " !! "
  if (!ret && all_punctuation(word))
    ret = true;

  // Check for too many apostraphes
  int num_apos = 0;
  for (int i = 0; i < strlen(cleaned); i++) {
    if (is_apostraphe(cleaned[i]))
      num_apos++;
  }
  if (num_apos > 1 && !all_punctuation(word))
    ret = false;

  free(cleaned);
  return ret;
}

/**
 * Loads dictionary into memory.  Returns true if successful else false.
 */
/**
 * Inputs:
 *  dictionary_file:    Path to the words file.
 *  hashtable:          The hash table to be populated.
 *
 * Returns:
 *  bool:       Whether or not the hashmap successfully populated.
 *
 * Modifies:
 *  hashtable: This hashmap should be filled with words from the file provided.
 *
 * Example:
 *  bool success = load_dictionary("wordlist.txt", hashtable);
 **/
bool load_dictionary(const char* dictionary_file, hashmap_t hashtable[])
{
  if (dictionary_file == NULL)
    return false;

  FILE *fp = fopen(dictionary_file, "r");
  if (fp == NULL)
    return false;

  for (int bucket = 0; bucket < HASH_SIZE; bucket++)
    hashtable[bucket] = NULL;

  char line[LENGTH];
  int bucket;
  int word_length;
  int count = 0;
  while (fgets(line, LENGTH, fp)) {
    node* new_node = calloc(1, sizeof(node));
    new_node->next = NULL;

    //Remove newline
    //https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
    word_length = strcspn(line, "\n");
    line[word_length] = '\0';

    strncpy(new_node->word, line, word_length);

    bucket = hash_function(line);

    if (hashtable[bucket] != NULL)
      new_node->next = hashtable[bucket];

    hashtable[bucket] = new_node;
    count++;
  }

  fclose(fp);
  return true;
}

/**
 * Array misspelled is populated with words that are misspelled. Returns the length of misspelled.
 */
/**
 * Inputs:
 *  fp:         A file pointer to the document to check for spelling errors.
 *  hashtable:  The hash table used to determine spelling
 *  misspelled: An empty char* array to be populated with misspelled words.
 *              This array will never be greater than 1000 words long.
 *
 * Returns:
 *  int:        The number of words in the misspelled arary.
 *
 * Modifies:
 *  misspelled: This array will be filled with misspelled words.
 *
 * Example:
 *  int num_misspelled = check_words(text_file, hashtable, misspelled);
 **/
int check_words(FILE* fp, hashmap_t hashtable[], char * misspelled[])
{
  if (fp == NULL || misspelled == NULL)
    return -1;

  int num_misspelled = 0;

  int max_line = 1024;
  char line[max_line];
  char *word;
  while (fgets(line, max_line, fp)) {
    word = strtok(line, " ");

    while (word != NULL) {
      if (!check_word(word, hashtable)) {
        // AFL if word was too long, malloc doesn't allocate enough memory
        // AFL check to make sure num_misspelled is less than the max
        if (num_misspelled < MAX_MISSPELLED)
          misspelled[num_misspelled] = prep_word(word);
        num_misspelled++;
      }

      word = strtok(NULL, " ");
    }
  }

  return num_misspelled;
}

/*
int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: ./spell_check dictionary file_to_check\n");
    return 1;
  }

  hashmap_t map[HASH_SIZE];
  //AFL was not checking loading success
  bool success = load_dictionary(argv[1], map);

  if (!success) {
    printf("Invalid dictionary file\n");
    return -1;
  }

  //AFL was not checking if this was a valid file
  FILE* check_this = fopen(argv[2], "r");
  if (check_this == NULL) {
    printf("Invalid file to check spelling for.\n");
    return -1;
  }

  char* misspelled[MAX_MISSPELLED];

  int wrong = check_words(check_this, map, misspelled);
  fclose(check_this);

  printf("%d words misspelled\n", wrong);
  destroy_dict(map);

  for (int i = 0; i < wrong; i++)
    free(misspelled[i]);

  return 0;
}
*/