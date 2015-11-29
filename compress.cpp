/*
 * UCLA CS 180 - Spring 2009
 * main function for compressing files via the Huffman alogrithm
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

static int MAX_BUF_SIZE = 101;			//changed to const
static int CHAR_BUF_SIZE = 64;			//moved from print_compressed_file, changed to static const
static int BIT_BUF_SIZE = 64;				//moved from print_compressed_file, changed to static const

/*
 * Main function for compressing
 */

struct node {struct node * parent; struct node * left_child; struct node * right_child; char char_value; int frequency;} node;
struct node_list {struct node * node; struct node_list * next_node_list;} node_list;
void print_node_list(struct node_list * front)
{
while(front != NULL)
	{
	printf("%i\t%c\t%i\n", (int)front->node->char_value, front->node->char_value, front->node->frequency);
	front = front->next_node_list;
	}
}

void count_occurrences(int char_occurrence[], char * file)
{

char buff[MAX_BUF_SIZE];
int i;
for(i=0; i<256; i++)
		char_occurrence[i] = 0;

	int read_marker = 0;

	ifstream fd(file);
	//int fd = open(file, O_RDONLY);

	if(!fd)//fd < 0)
		{
		printf("\ninput file %s does not exist\n\n", file);
		exit(0);
		}
	
	while (1)
		{
		fd.read(buff, 100); //changed read_marker = read(fd, buf, 100);
		read_marker = fd.gcount();
		if (read_marker <= 0)
			break;
		buff[read_marker] = '\0';		
		
			for(i = 0; buff[i] != '\0'; i++)
				char_occurrence[(int) buff[i]] = char_occurrence[(int) buff[i]] + 1;
		}

	fd.close();
}


struct node_list * construct_node_list(int char_occurrence[])
{
int i;
struct node_list * list_front = (struct node_list *)malloc(sizeof (struct node_list)); //added explicit cast
for(i=0; char_occurrence[i] == 0 && i < 256; i++)
	;
if(i == 256)
	return NULL;
list_front->node = (struct node *) malloc(sizeof(struct node)); //added explicit cast
list_front->node->parent = NULL;
list_front->node->left_child = NULL;
list_front->node->right_child = NULL;
list_front->node->char_value = (char) i;
list_front->node->frequency = char_occurrence[i];
list_front->next_node_list = NULL;
list_front->next_node_list = NULL;
struct node_list * list_back = list_front;

i++; //so we don't count this one twice...

for(; i < 256; i++)
	{
	if(char_occurrence[i] == 0)
		continue;	//don't include anything that never appears XD
	struct node_list * new_node_list = (struct node_list *) malloc(sizeof (struct node_list)); //added explicit cast
	new_node_list->node = (struct node *)malloc(sizeof (struct node)); //added explicit cast
	new_node_list->node->char_value = (char) i;
	new_node_list->node->frequency = char_occurrence[i];
	new_node_list->node->left_child = NULL;
	new_node_list->node->right_child = NULL;
	new_node_list->node->parent = NULL;
	new_node_list->next_node_list = NULL; //visual c++ doesn't auto-zero pointers
	//new node should be all created.  Now we insert it sorted.
	struct node_list * temp = list_front;
	struct node_list * prev_node = NULL;
	while(temp->node->frequency < new_node_list->node->frequency)
		{
		prev_node = temp;
		temp = temp->next_node_list;
		if(temp == NULL)
			break;
		}
		
	//prev_node should point to the first item in the list that new_node_list should be placed behind of (or NULL if temp should be the first node)
	
	if(prev_node == NULL)
		{
		new_node_list->next_node_list = list_front;
		list_front = new_node_list;
		}
	else if (prev_node == list_back)
		{
		list_back->next_node_list = new_node_list;
		list_back = new_node_list;
		}
	else
		{
		new_node_list->next_node_list = temp;
		prev_node->next_node_list = new_node_list;
		}
	}			
	//... node list should now be ordered

return list_front;
}

struct node * construct_node_tree(struct node_list * list_front)
{
	if(list_front == NULL)
		return NULL;
	if(list_front->next_node_list == NULL)
		return list_front->node;
	
	struct node_list * new_node_list = (struct node_list *) malloc(sizeof(struct node_list)); //added explicit cast
	new_node_list->node = (struct node *)malloc(sizeof(struct node)); //added explicit cast
	new_node_list->node->parent = NULL;
	new_node_list->next_node_list = NULL; //visual c++ doesn't auto-zero out pointers
	new_node_list->node->left_child = list_front->node;
	list_front->node->parent = new_node_list->node;
	new_node_list->node->right_child = list_front->next_node_list->node;
	list_front->next_node_list->node->parent = new_node_list->node;
	new_node_list->node->frequency = new_node_list->node->left_child->frequency + new_node_list->node->right_child->frequency;
	list_front = list_front->next_node_list->next_node_list;
	if(list_front == NULL)
		return new_node_list->node;
	
	struct node_list * temp = list_front;
	struct node_list * prev_node = NULL;
		
	while(temp->node->frequency < new_node_list->node->frequency)
		{
		prev_node = temp;
		temp = temp->next_node_list;
		if(temp == NULL)
			break;
		}
		
	//prev_node should point to the first item in the list that new_node_list should be placed behind of (or NULL if temp should be the first node)
	
	if(prev_node == NULL)
		{
		new_node_list->next_node_list = list_front;
		list_front = new_node_list;
		}
	else
		{
		new_node_list->next_node_list = temp;
		prev_node->next_node_list = new_node_list;
		}
	
	return construct_node_tree(list_front);			
}

void edit_node_tree_for_zeroes( struct node* root)
{
//used to insure that no real character maps to all 0's (so we don't have extra characters at the end when we decompress)
struct node * current_node = root;
while(current_node->left_child != NULL)
	current_node = current_node-> left_child;
	
struct node* left_node = (struct node *)malloc(sizeof(struct node)); //added explicit cast
left_node->parent = current_node;
left_node->left_child = NULL;
left_node->right_child = NULL;
left_node->frequency = 0;
left_node->char_value = '\0';
struct node* right_node = (struct node *)malloc(sizeof(struct node)); //added explicit cast
right_node->parent = current_node;
right_node->left_child = NULL;
right_node->right_child = NULL;
right_node->frequency = current_node->frequency;
right_node->char_value = current_node->char_value;
current_node->frequency = 0;
current_node->char_value = '\0';
current_node->left_child = left_node;
current_node->right_child = right_node;

}
void print_node_tree(struct node * root, char code[], int length, char** compression_array, FILE* outfile)
{
if (root == NULL)
	return;
if (root->left_child == NULL)
	{
	fprintf(outfile, "%s %c\n", code, root->char_value);
	compression_array[(int) (root->char_value)] = (char *)malloc((strlen(code) + 1) * sizeof(char)); //added explicit cast
	strcpy(compression_array[(int) root->char_value], code);
	return;
	}
code[length] = '0';
code[length + 1] = '\0';
print_node_tree(root->left_child, code, length + 1, compression_array, outfile);
code[length] = '1';
code[length + 1] = '\0';
print_node_tree(root->right_child, code, length +1, compression_array, outfile);
}

void print_compressed_file(char ** compression_array, char * in, FILE* outfile)
{
	
	int read_marker = 0;
	char char_buf[CHAR_BUF_SIZE];
	char bit_buf[BIT_BUF_SIZE + 1];
	char_buf[CHAR_BUF_SIZE -1] = '\0';
	int bit_offset;

	for(bit_offset = 0; bit_offset <= BIT_BUF_SIZE; bit_offset ++)
		bit_buf[bit_offset] = 0;
	bit_offset = 0;

	FILE* infile = fopen(in,"r");
	
	if(infile == NULL)
		{
		printf("\nproblem opening %s for second read...\n\n", in);
		exit(0);
		}

	while (1)
		{
		read_marker = fread(char_buf, 1, CHAR_BUF_SIZE -1, infile);
		if (read_marker <= 0)
			break; // we're done
		char_buf[read_marker] = 0;
		int i,j;
		for(i = 0; i< read_marker; i++)
			{
			for(j = 0; j < strlen(compression_array[(int) char_buf[i]]); j++)
				{
				if(compression_array[(int) char_buf[i]][j] == '1')
					bit_buf[bit_offset/8] |= (1 << (7 - (bit_offset % 8)));	
				else
					/*do nothing, since we already set the array to all 0's by default*/;	
				bit_offset++;
				if(bit_offset == 8 * (BIT_BUF_SIZE))
					{
					fwrite(bit_buf, 1 , bit_offset/8, outfile);
					for(bit_offset = 0; bit_offset <= BIT_BUF_SIZE; bit_offset ++)
						bit_buf[bit_offset] = 0;
					bit_offset = 0;
					}
				}
			}
					
		}
		//we'd better output what we have left after we finish the while loop...
		fwrite(bit_buf, 1, (bit_offset -1)/8 + 1, outfile);

}

void print_compression_array( char ** compression_array)
{
int i;
for(i = 0; i<256; i++)
	if(compression_array[i] != NULL)
		printf("%c %s\n", (char) i, compression_array[i]);
		
}

int
main(int argc, char *argv[])
{

	int char_occurrence[256];
	int i = 0;


	if (argc !=3)
		{
		printf("\nWrong number of arguments- expected \"./compress inputfile outputfile\"\n\n");
		exit(0);
		}
	FILE *outfile;
	outfile = fopen(argv[2], "w" );
	if(outfile == NULL)
		{
		printf("error opening output file\n");
		exit(0);
		}
	
	count_occurrences(char_occurrence, argv[1]);
	
	struct node_list * list_front = construct_node_list(char_occurrence);
	//print_node_list(list_front); //diagnostic function
	
	struct node * root = construct_node_tree(list_front);
	edit_node_tree_for_zeroes(root); //makes sure nothing maps to all 0's
	
	char** compression_array = (char **) malloc(256* sizeof (char *)); //zero out the array for storing the bitcode of each char value in my compression
	for(i = 0; i<256; i++)
		compression_array[i]= NULL;
				
	char code[300];
	print_node_tree(root, code, 0, compression_array, outfile); //prints the node tree to the compressed file for the decompressor to read
	//print_compression_array(compression_array); // diagnostic function
	fprintf(outfile, "---\n"); //indicates the end of the node tree, and the beginning of the compressed data
	print_compressed_file(compression_array, argv[1], outfile); //prints the compressed version of the file
	fclose(outfile);  //woot
	printf("done\n"); //wooter
	
}
