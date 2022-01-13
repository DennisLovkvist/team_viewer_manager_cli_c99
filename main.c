#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>
#include <locale.h>
#include <unistd.h>


#define MAX_DEPTH 6
#define BUFFER_LENGTH_PATH 144
#define BUFFER_LENGTH_NAME 24

typedef struct Node
{    
    char console_output_line[BUFFER_LENGTH_PATH];
    char path[BUFFER_LENGTH_PATH];
    char name[BUFFER_LENGTH_NAME];
    char ip[15];
    char twid[15];
    char pwd[16];
    Node* parent;
    Node* children[64];    
    int child_count;
    int child_index;
    int chars_in_name;
    bool is_endpoint;
    bool is_collapsed;
    bool generated_console_output_line;
}
Node;

void genereate_line(char *str,int position, Node *node)
{
    if(node->parent == NULL)return;
    if(node->parent->parent == NULL)
    {
        for (size_t i = 0; i < 7; i++)
        {
            str[position--] = ' ';
        } 
        return;
    }
    //+3 for child count indicator example: [3]
    size_t length = (node->parent->chars_in_name)-1 + 3;
    for (size_t i = 0; i < length; i++)
    {
        str[position--] = ' ';
    } 
    str[position--] = (node->parent->child_index == node->parent->parent->child_count-1) ? ' ' :'|';  

    if(position > 0)
    {
        genereate_line(str,position,node->parent);
    }
    
}
void calc_length(Node *node,int *length)
{ 
    if(node->parent == NULL)return;
    //+3 for child count indicator example: [3]
    *length += (node->parent->chars_in_name) + 3;
    calc_length(node->parent,length);
}
void clear_tree(WINDOW *win,char * clear_line, int lines)
{
    for (size_t i = 1; i < (size_t)lines; i++)
    {        
        mvwprintw(win,i,1,clear_line);
    }
    
}

void print_tree(WINDOW *win,Node *node, int depth,int *line_nr, Node *selected_node)
{    
    char *output = node->console_output_line;     

    if(!node->generated_console_output_line)
    {
        int length = 0;   
        
        calc_length(node,&length); 

        genereate_line(output,length-1,node);

        int P = length;
        int namen_length = strlen(node->name);

        if(node->parent == NULL)
        {
            memcpy(output+P,"└─",6);
            P += 6;
        }
        else
        {
            if(node->child_index == node->parent->child_count-1)
            {         
                memcpy(output+P,"└─",6);
                P += 6;   
            }
            else
            {     
                memcpy(output+P,"├─",6);
                P += 6;     
            }
        }

        memcpy(output+P,node->name,namen_length);
        P += namen_length;

        memcpy(output+P,"[",1);
        P+=1;

        char str[2];
        sprintf(str, "%d", node->child_count);
        memcpy(output+P,str,1); 
        P += 1;

        memcpy(output+P,"]",1);   

        output[P+1] = '\0';

        node->generated_console_output_line = true;

    }

   
    
   
    if(node == selected_node)
    {
        wattron(win, COLOR_PAIR(1));
    }
    else
    {
        wattroff(win, COLOR_PAIR(1));
    }

     mvwprintw(win,*line_nr,1,output);

    
     *line_nr +=1;

    if(!node->is_collapsed)
    {
        if(node->child_count > 0)
        {
            for (int i = 0; i < node->child_count; i++)
            {
                print_tree(win,node->children[i],depth+1,line_nr,selected_node);
            }
        }
    }
}
void count_utf8_chars(const char *str,int *count)
{
    *count = 0;
    int position = 0;
    while(str[position] != '\0')
    {
        if((str[position] & 0xc0) != 0x80)
        {
            *count+=1;
        }
        position ++;
    }
}
void parse_node_tree(int *path_endings,int path_index, const char *line, Node *nodes, int *node_count)
{
    for (size_t i = 0; i < path_index; i++)
    {
        Node *node = &nodes[*node_count];
        
        int temp_length = (path_endings[i+1]) - (path_endings[i]+1);
        int length_name = temp_length >= BUFFER_LENGTH_NAME ? BUFFER_LENGTH_NAME-1 : temp_length;
        int length_path = path_endings[i+1] >= BUFFER_LENGTH_PATH ? BUFFER_LENGTH_PATH-1 : path_endings[i+1];

        if(length_name > 0)
        {
            memcpy(node->name,&line[(path_endings[i]+1)],sizeof(char)*length_name);
            node->name[length_name] = '\0';
            count_utf8_chars(node->name,&node->chars_in_name);
        }
        if(length_path > 0)
        {
            memcpy(node->path,&line[0],sizeof(char)*length_path);
            node->path[length_path] = '\0';
        }
        else
        {
            memcpy(node->path,"/root",sizeof(char)*5);
            node->path[5] = '\0';
        }  
        bool exists = false;
        for (int j = (*node_count)-1; j >= 0; j--)
        {
            if(strcmp(nodes[j].path,node->path) == 0)
            {
                 exists = true;
                 break;
            }
        }       
        
        if(!exists)
        {
            node->child_count = 0;
            node->is_collapsed = true;
            node->generated_console_output_line = false;

            for (int j = (*node_count)-1; j >= 0; j--)
            {
                if(strncmp(nodes[j].path,node->path,strlen(nodes[j].path)) == 0)
                {
                    
                    nodes[j].children[nodes[j].child_count] = node; 
                    node->child_index = nodes[j].child_count;
                    nodes[j].child_count++;
                    node->parent = &nodes[j];
                    break;
                }                
            }
            *node_count += 1;
        }
    }
    
}
int main () 
{
    setlocale(LC_ALL, "sv_SE.UTF-8");  
    initscr();
    start_color();    
    init_pair(1,COLOR_GREEN,COLOR_BLACK);

    int max_y,max_x;
    getmaxyx(stdscr,max_y,max_x);

    WINDOW *win = newwin(max_y,(max_x <= BUFFER_LENGTH_PATH+1) ? max_x:BUFFER_LENGTH_PATH+1,0,0);
    noecho();
    curs_set(0);
    keypad(win, TRUE);
    box(win,0,0);

    char *clear_line = (char*)malloc(sizeof(char)*BUFFER_LENGTH_PATH);
    memset(clear_line,' ',BUFFER_LENGTH_PATH-1);

    FILE *data = fopen("src.txt", "r");

    if (data != NULL)
    {
        char * line = NULL;
        size_t len = 0;
        ssize_t read;
        int max_nodes = 0;  
        int c; 
         for (c = getc(data); c != EOF; c = getc(data))
         {
            if (c == '/')
            {
                max_nodes ++;
            }

        }
        rewind(data);

        Node *nodes = (Node*)malloc(sizeof(Node)*max_nodes);

        strcpy(nodes[0].name,"root");
        strcpy(nodes[0].path,"/root");
        count_utf8_chars(nodes[0].name,&(nodes[0].chars_in_name));
        nodes[0].is_collapsed = false;
        nodes[0].child_count = 0;
        nodes[0].generated_console_output_line = false;
        nodes[0].parent = NULL;
        int node_count = 1;        

        while ((read = getline(&line, &len, data)) != -1) 
        {
            int path_end = 0;

            bool address_parsed = false,ip_parsed = false,twid_parsed = false,pwd_parsed = false;  
            int path_endings[6] = {0,0,0,0,0,0};
            int parsing_stage = 0, path_index = 0;

            for (size_t i = 0; line[i]; i++)
            {
                if(line[i] == ';')
                {
                    Node *node = &nodes[node_count-1];
                    int start,end,length = 0;
                    switch (parsing_stage)
                    {
                        case 0:
                            path_endings[path_index] = i;
                            parse_node_tree(path_endings,path_index,line,nodes,&node_count);                            
                            break;
                        case 1:
                            start = path_endings[path_index]+1;
                            end = i;
                            length = end - start;
                            memcpy(node->ip,&line[start],length);
                            node->ip[length] = '\0';
                            break;
                        case 2:
                            start = end+1;
                            end = i;
                            length = end - start;
                            memcpy(node->twid,&line[start],length);
                            node->twid[length] = '\0';
                            break;
                        case 3:
                            start = end+1;
                            end = i;
                            length = end - start;
                            memcpy(node->pwd,&line[start],length);
                            node->pwd[length] = '\0';
                            break;                       
                        default:
                            break;
                    }

                    parsing_stage ++; 
                }
                else if(parsing_stage == 0)
                {
                    if(line[i] == '/')
                    {
                        path_endings[path_index] = i;
                        path_index ++;
                    }
                }
            }
        }

    
        



    
        int line_nr = 1;
        Node *snode = &nodes[0];
        print_tree(win,&nodes[0],1,&line_nr,snode);
       
        wrefresh(win);

        bool quit = false;

        while(!quit)
        {
            int key = wgetch(win);

            if(key == KEY_DOWN)
            {
                if(snode->parent != NULL)
                {
                    line_nr = 1;
                    snode = (snode->child_index +1 >= snode->parent->child_count) ? snode->parent->children[0] : snode->parent->children[snode->child_index+1];
                    print_tree(win,&nodes[0],1,&line_nr,snode);


                    mvwprintw(win,0,0,snode->ip);
                    wrefresh(win);
                }
            }
            else if(key == KEY_UP)
            {
                if(snode->parent != NULL)
                {
                    line_nr = 1;
                    snode = (snode->child_index -1 < 0) ? snode->parent->children[snode->parent->child_count-1] : snode->parent->children[snode->child_index-1];
                    print_tree(win,&nodes[0],1,&line_nr,snode);


                    wrefresh(win);
                }
            }
            else if(key == KEY_LEFT)
            {
                if(snode->parent != NULL)
                {
                    snode = snode->parent;
                    snode->is_collapsed = true;
                    clear_tree(win,clear_line,line_nr);
                    line_nr = 1;
                    print_tree(win,&nodes[0],1,&line_nr,snode);


                    wrefresh(win);
                }
            }
            else if(key == KEY_RIGHT)
            {      
                if(snode->child_count > 0)
                {          
                    snode->is_collapsed = false;
                    snode = snode->children[0];
                    clear_tree(win,clear_line,line_nr);
                    line_nr = 1;
                    print_tree(win,&nodes[0],1,&line_nr,snode);


                    wrefresh(win);
                }
            }
            else if(key == 10)
            {
                quit = true;
            }
            sleep(0.1);

       }
       

    }

    free(clear_line);
    //endwin();
    fclose(data);
    return(0);
}