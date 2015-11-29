/*
 * UCLA CS 180 - Spring 2009
 * main function for decompressing files via the Huffman alogrithm
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

static const int MAX_BUF_SIZE = 256; //moved declaration, made static const
static const int CHAR_BUF_SIZE = 64; //moved declaration, made static const
static const int BIT_BUF_SIZE = 64;	 //moved declaration, made static const

struct node {struct node * parent; struct node * left_child; struct node * right_child; char char_value; int frequency;} node;
struct node_list {struct node * node; struct node_list * next_node_list;} node_list;
void print_node_tree(struct node * root, char code[], int length, char** compression_array)
{
	if (root == NULL)
		return;
	if (root->left_child == NULL)
		{
		if(root->char_value == '\n')
			printf("%s \\n\n", code);
		else
			printf("%s %c\n", code, root->char_value);
		compression_array[(int) (root->char_value)] = (char *)malloc((strlen(code) + 1) * sizeof(char)); //added explicit cast
		strcpy(compression_array[(int) root->char_value], code);
		return;
		}
	code[length] = '0';
	code[length + 1] = '\0';
	print_node_tree(root->left_child, code, length + 1, compression_array);
	code[length] = '1';
	print_node_tree(root->right_child, code, length +1, compression_array);
}

struct node * construct_node_tree (FILE* infile)
{
	char buf[MAX_BUF_SIZE];
	struct node * root = (struct node*) malloc(sizeof(struct node)); //added explicit cast
	root->parent = NULL;
	root->right_child = NULL;
	root->left_child = NULL;

	struct node * current_node;
	fgets(buf, MAX_BUF_SIZE, infile);
	int i;
	int j = 0;
	while(buf[0] != '-' || buf [1] != '-' || buf[2] != '-') //\n\r") != 0) //changed to because of binary file newline headache
	{
	current_node = root;
	for(i = 0; buf[i] != ' '; i++)
		{
		if(buf[i] == '0')
			{
			if(current_node->left_child == NULL)
				{
				current_node->left_child = (struct node *) malloc(sizeof(struct node)); //added explicit cast
				current_node->left_child->parent = current_node;
				current_node = current_node->left_child;
				current_node->left_child = NULL;
				current_node->right_child = NULL;
				}
			else
				current_node = current_node->left_child;
			}
		else
			{
			if(current_node->right_child == NULL)
				{
				current_node->right_child = (struct node *) malloc(sizeof(struct node)); //added explicit cast
				current_node->right_child->parent = current_node;
				current_node = current_node->right_child;
				current_node->right_child = NULL;
				current_node->left_child = NULL;
				}
			else
				current_node = current_node->right_child;
			}
		}
		i++;
		if(buf[i] != '\r')
			current_node->char_value = buf[i];
		else
			{
			//used to just have if(buf[i] == '\r') statement - changed to this, and changed to
			//\r instead of \n, because binary files suck
			current_node->char_value = '\n';
			fgets(buf, MAX_BUF_SIZE, infile);
			}
		fgets(buf, MAX_BUF_SIZE, infile);
		i = 0;
	}
	return root;
}
//int get_char_from_input_stream(struct node * root, char bit_buf[], int bit_offset)
//{

//}
void print_decompressed_file(struct node * root, FILE* infile, FILE* outfile)
{
	int read_marker = 0;
	char char_buf[CHAR_BUF_SIZE +1];
	char bit_buf[BIT_BUF_SIZE+1];
	int bit_offset = 0;
	struct node* current_node = root;
	int k;
	for(k=0; k < CHAR_BUF_SIZE; k++)
		char_buf[k] = 0;
	int char_buf_offset = 0;
	while (1)
		{
		read_marker = fread(bit_buf, 1, BIT_BUF_SIZE, infile);
		if (read_marker <= 0)
			break;
			
		bit_buf[read_marker] = 0;
	
		for(bit_offset = 0; bit_offset < 8 * read_marker; bit_offset++)
			{
			if(current_node == NULL)
				printf("huge error!");
			if(current_node->left_child == NULL)	//we are a leaf node
				{
				if(current_node->right_child != NULL) //um... something went wrong
					printf("huge error!");
				char_buf[char_buf_offset] = current_node->char_value;
				char_buf_offset++;
				if(char_buf_offset == CHAR_BUF_SIZE-1)
					{
					fprintf(outfile,"%s",char_buf);
					char_buf_offset = 0;
					for(k=0; k < CHAR_BUF_SIZE; k++)
						char_buf[k] = 0;	
					}
				current_node = root;			
					
				}
			k = bit_buf[bit_offset/8] & (1 << (7 - (bit_offset % 8)));
			if(k)
				current_node = current_node->right_child;	
			else
				current_node = current_node->left_child;

			}
			
		}
		fprintf(outfile, "%s", char_buf);
		
		
}

int
main(int argc, char *argv[])
{

	if (argc !=3)
		{
		printf("\nWrong number of arguments- expected \"./compress inputfile outputfile\"\n\n");
		exit(0);
		}
	FILE *outfile = fopen(argv[2], "w" ); //b added to make it binary because default in visual studio is ascii
	if(outfile == NULL)
		{
		printf("error opening output file\n");
		exit(0);
		}

	FILE* infile = fopen(argv[1], "rb+"); //b added to make it binary because default in visual studio is ascii
	if(infile == NULL)
		{
		printf("error opening input file\n");
		exit(0);
		}

	struct node * root = construct_node_tree(infile);
	char code[300];
	char** compression_array = (char **)malloc(256* sizeof (char *)); //added explicit cast
	
	//print_node_tree(root, code, 0, compression_array);

	print_decompressed_file(root, infile, outfile);
	printf("done\n");
	
}
