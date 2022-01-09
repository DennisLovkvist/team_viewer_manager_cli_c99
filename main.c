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
        for (int i = 0; i < node->child_count; i++)
        {
            print_tree(win,node->children[i],depth+1,line_nr,selected_node);
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

        Node nodes[max_nodes];
        strcpy(nodes[0].name,"root");
        strcpy(nodes[0].path,"root");
        count_utf8_chars(nodes[0].name,&(nodes[0].chars_in_name));
        nodes[0].is_collapsed = false;
        nodes[0].generated_console_output_line = false;
        int node_count = 1;

        

        while ((read = getline(&line, &len, data)) != -1) 
        {

            int path_end = 0;
            Node *endpoint_node = NULL;

            bool address_parsed = false;
            bool ip_parsed = false;
            bool twid_parsed = false;
            bool pwd_parsed = false;

            int start = 0;
            int end = 0;
            //int path_end = 0;
            for (size_t i = 0; line[i]; i++)
            {
                Node *node = &nodes[node_count];

                if(!address_parsed)
                {                    
                    if(line[i] == '/')
                    {         
                        //Previous Node Path
                        int length_path = end + 1 >= BUFFER_LENGTH_PATH ? BUFFER_LENGTH_PATH-1 : end + 1;
                        char prev[length_path];     
                        memcpy(prev,&line[0],length_path);
                        prev[length_path-1] = '\0';

                        if(end == 0)
                        {
                            strcpy(prev,"root");
                        }

                        end = i;

                        //Node Name
                        int length_name = end-start + 1 >= BUFFER_LENGTH_NAME ? BUFFER_LENGTH_NAME-1 : end-start + 1;
                        memcpy(node->name,&line[start],length_name);
                        node->name[length_name-1] = '\0';
                        count_utf8_chars(node->name,&node->chars_in_name);

                        //Current Node Path
                        length_path = end + 1 >= BUFFER_LENGTH_PATH ? BUFFER_LENGTH_PATH-1 : end + 1;
                        memcpy(node->path,&line[0],length_path);
                        node->path[length_path-1] = '\0';
                        node->is_collapsed = true;
                        node->generated_console_output_line = false;

                        //Look if Node allready exists
                        bool exists = false;
                        for (int j = 0; j < node_count; j++)
                        {
                            if(strcmp(nodes[j].path,node->path) == 0)
                            {
                                exists = true;   
                                break;
                            }
                        }    

                        //Adds Node if no other with the same path exists
                        if(!exists)
                        {
                            node->child_count = 0;

                            for (unsigned int j = node_count; j != 0; j--)
                            {                
                                int index = j-1;
                                if(strcmp(nodes[index].path,prev) == 0)
                                {
                                    nodes[index].children[nodes[index].child_count] = node; 
                                    node->child_index = nodes[index].child_count;
                                    nodes[index].child_count++;
                                    node->parent = &nodes[index];
                                    break;
                                }
                            } 

                            node_count++;  
                        }
                        start = end+1;                    
                    }    
                    else if(line[i] == ';')
                    {
                        path_end = start-1;
                        end = i;

                        endpoint_node = &nodes[node_count];

                        if(end-start == 0)
                        {
                            memcpy(endpoint_node->name,"default",7);
                        }
                        else
                        {
                            //Node name (Endpoint)
                            int length_name = end-start+1 >= BUFFER_LENGTH_NAME ? BUFFER_LENGTH_NAME-1 : end-start+1;
                            memcpy(endpoint_node->name,&line[start],length_name);
                            endpoint_node->name[length_name-1] = '\0';
                            count_utf8_chars(endpoint_node->name,&endpoint_node->chars_in_name);
                            endpoint_node->is_endpoint = true;  
                            endpoint_node->is_collapsed = true;
                            endpoint_node->generated_console_output_line = false;

                            //Node path (Endpoint)
                            int length_path = path_end+1 >= BUFFER_LENGTH_PATH ? BUFFER_LENGTH_PATH-1 : path_end+1;
                            char path[length_path];
                            memcpy(path,&line[0],length_path-1);     
                            path[length_path-1] = '\0';

                            for (unsigned int j = node_count; j != 0; j--)
                            {     
                                int index = j-1;           
                                Node *local_node = &nodes[index];
                                if(strcmp(local_node->path,path) == 0)
                                {
                                    if(!local_node->is_endpoint)
                                    {
                                        local_node->children[local_node->child_count] = endpoint_node;
                                        endpoint_node->child_index = local_node->child_count;
                                        local_node->child_count++;
                                        endpoint_node->parent = local_node;
                                        break;
                                    }
                                }
                            }   
                        }  

                        node_count++;  
                        address_parsed = true;

                        start = end+1; 
                    }
                }
                else if(!ip_parsed)
                {
                    if(line[i] == ';')
                    {
                        end = i;
                        int len = (end-start < 14) ? end-start:14;
                        memcpy(endpoint_node->ip,&line[start],len);
                        printf("%s",endpoint_node->name);
                        printf("%s",endpoint_node->ip);
                        ip_parsed = true;
                        start = end+1;
                    }
                }
                else if(!twid_parsed)
                {
                    if(line[i] == ';')
                    {
                        end = i;
                        int len = (end-start < 9) ? end-start:9;
                        memcpy(endpoint_node->twid,&line[start],len);
                        twid_parsed = true;
                        start = end+1;
                    }
                }
                else if(pwd_parsed)
                {
                    if(line[i] == ';')
                    {
                        end = i;
                        int len = (end-start < 15) ? end-start:15;
                        memcpy(endpoint_node->pwd,&line[start],len);
                        pwd_parsed = true;
                        start = end+1;
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