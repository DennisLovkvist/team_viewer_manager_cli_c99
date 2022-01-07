#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>
#include <locale.h>

#define BUFFER_LENGTH_PATH 64
#define BUFFER_LENGTH_NAME 32

typedef struct Node
{
    char path[BUFFER_LENGTH_PATH];
    char name[BUFFER_LENGTH_NAME];
    Node* parent;
    Node* children[64];    
    int child_count;
    bool endpoint;
    bool collapsed;
    int child_index;
    int chars_in_name;
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
    int length = (node->parent->chars_in_name)-1 + 3;
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
void clear_tree(WINDOW *win, int lines)
{
    char lol[64];
    memset(lol,' ',63);
    for (size_t i = 1; i < lines; i++)
    {        
        mvwprintw(win,i,1,lol);
    }
    
}
void print_tree(WINDOW *win,Node *node, int depth,int *line_nr, Node *selected_node)
{
    char output[80];
    memset(output,' ',80);

    int length = 0;   
    calc_length(node,&length);   
    output[79] = '\0';  
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
    P+=1;

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

    if(!node->collapsed)
    {
        for (size_t i = 0; i < node->child_count; i++)
        {
            print_tree(win,node->children[i],depth+1,line_nr,selected_node);
        }
    }
}
void count_utf8_chars(char *str,int *count)
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

    FILE *data = fopen("src.txt", "r");
 
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    initscr();

start_color();

    init_pair(1,COLOR_GREEN,COLOR_BLACK);
        WINDOW *win = newwin(60,80,0,0);
    noecho();
    curs_set(0);
keypad(win, TRUE);
    int max_y,max_x;
    getmaxyx(stdscr,max_y,max_x);

    box(win,0,0);



    if (data != NULL)
    {
        int max_nodes = 0;  
        char c; 
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
        nodes[0].collapsed = false;
        int node_count = 1;

        Node endpoints[max_nodes];
        int endpoint_count = 0;

        int path_end = 0;

        while ((read = getline(&line, &len, data)) != -1) 
        {
            int start = 0;
            int end = 0;
            for (size_t i = 0; line[i]; i++)
            {
                Node *node = &nodes[node_count];
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
                    node->collapsed = true;

                    //Look if Node allready exists
                    bool exists = false;
                    for (size_t j = 0; j < node_count; j++)
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
                    path_end = i;
                    break;
                }
            
            }       

            Node *endpoint = &nodes[node_count];


            //Node name (Endpoint)
            int length_name = path_end-start+1 >= BUFFER_LENGTH_NAME ? BUFFER_LENGTH_NAME-1 : path_end-start+1;
            memcpy(endpoint->name,&line[start],length_name);
            endpoint->name[length_name-1] = '\0';
            count_utf8_chars(endpoint->name,&endpoint->chars_in_name);
            endpoint->endpoint = true;  

            //Node path (Endpoint)
            int length_path = end+1 >= BUFFER_LENGTH_PATH ? BUFFER_LENGTH_PATH-1 : end+1;
            char path[length_path];
            memcpy(path,&line[0],length_path-1);     
            path[length_path-1] = '\0';

            for (unsigned int i = node_count; i != 0; i--)
            {     
                int index = i-1;           
                Node *node = &nodes[index];
                if(strcmp(node->path,path) == 0)
                {
                    if(!node->endpoint)
                    {
                        node->children[node->child_count] = endpoint;
                        endpoint->child_index = node->child_count;
                        node->child_count++;
                        endpoint->parent = node;
                        break;
                    }
                }
            }     
            node_count++;      
        }

        int line_nr = 1;

        Node *snode = &nodes[0];

        print_tree(win,&nodes[0],1,&line_nr,snode);

        wrefresh(win);
        char ch;
        while(1)
        {
            int key = wgetch(win);

            if(key == KEY_DOWN)
            {
                if(snode->parent != NULL)
                {
                    line_nr = 1;
                    snode = (snode->child_index +1 >= snode->parent->child_count) ? snode->parent->children[0] : snode->parent->children[snode->child_index+1];
                    print_tree(win,&nodes[0],1,&line_nr,snode);
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

            if(key == KEY_LEFT)
            {
                if(snode->parent != NULL)
                {
                    snode = snode->parent;
                    snode->collapsed = true;
                    clear_tree(win,line_nr);
                    line_nr = 1;
                    print_tree(win,&nodes[0],1,&line_nr,snode);
                    wrefresh(win);
                }
            }

            else if(key == KEY_RIGHT)
            {      
                if(snode->child_count > 0)
                {          
                    snode->collapsed = false;
                    snode = snode->children[0];
                    clear_tree(win,line_nr);
                    line_nr = 1;
                    print_tree(win,&nodes[0],1,&line_nr,snode);
                    wrefresh(win);
                }
            }
        }
    }
    endwin();
    fclose(data);
    return(0);
}