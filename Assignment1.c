/**
 * A command line utility that takes any number of words on the command
 * line and prints the words out. The first argument is a character index
 * by which to sort the words.
 */
#include <stdio.h>
#include <stdlib.h>

void swap_words(char** words, int index_word_1, int index_word_2);
int  is_bigger(char* pointer_word_1, char* pointer_word_2, int position);
char** sort(int number_words, char** words, int position);

int main(int argc, char **argv) {
	if(argc < 3) {
		printf("Insufficient arguments\n");
		return -1;
	}

	int position = atoi(*(argv+1));
	sort(argc-2, argv+2, position);
	for(int i=2; i<argc; ++i) {
		printf("%s\n", *(argv+i));
	}
	return 0;
}

/**
 * swap_word swaps two strings in an array of strings
 * @param words an array of strings
 * @param index_word_1 index of the first word
 * @param index_word_2 index of the second word
 */
void swap_words(char **words, int index_word_1, int index_word_2) {
	char **word1 = (words+index_word_1);
	char **word2 = (words+index_word_2);
	char *tmp = *word1;
	*word1 = *word2;
	*word2 = tmp;
}

/**
 * is_bigger returns true if the character at the given index in the first word
 * is alphanumerically greater than the character in the second word.
 * @param pointer_word_1 a character string
 * @param pointer_word_2 a character string
 * @param position a character index
 * @param true if the character in the first word is greater than the second
 */
int  is_bigger(char* pointer_word_1, char* pointer_word_2, int position) {
	return *(pointer_word_1+position) > *(pointer_word_2+position);
}

/**
 * sort sorts an array of strings by a character index using bubble sort
 * @param num_words the number of elements in words
 * @param words an array of strings
 * @param position a character index
 * @return the sorted array
 */
char** sort(int number_words, char** words, int position) {
	for(int i=0; i<number_words-1; ++i) {
		for(int j=0; j<number_words-1; ++j) {
			if(is_bigger(*(words+j), *(words+j+1), position)) {
				swap_words(words, j, j+1);
			}
		}
	}
	return words;
}
