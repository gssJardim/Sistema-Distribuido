// Grupo 55
// Jose Alves nº 44898
// Gustavo Jardim nº 48483
// Henrique Lopes nº 52840

#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"

/*
 * Tree structure.
 *
 * Members:
 *      p_root: First node.
 *      size: Number of nodes on the gp_TREE.
 */
struct tree_t
{
    struct node_t* p_root;
    size_t size;
};

/*
 * Node structure.
 *
 * Members:
 *      p_entry: Entry kept by the node.
 *      p_left: Left child.
 *      p_right: Right child.
 */
struct node_t
{
    struct entry_t* p_entry;
    struct node_t* p_left;
    struct node_t* p_right;
};

/*
 * Funcao que cria um novo node, alocando a memoria necessaria.
 *
 * Parameters:
 *      p_entry: Entry to be kept by the node.
 *
 * Returns:
 *      Pointer to the new node. NULL on error.
 */
struct node_t* node_create( struct entry_t* p_entry );

/*
 * Funcao que elimina um node, libertando toda a memória por ele ocupada.
 *
 * Parameters:
 *      p_node: Node to be destroyed.
 *
 */
void node_destroy( struct node_t* p_node );


/*
 * Funcao que elimina um node e todos os filhos desse mesmo node, libertando toda a memória por eles ocupados.
 *
 * Parameters:
 *      p_node: Node (and children) to be destroyed.
 *
 */
void node_destroy_recursive( struct node_t *p_node );


/*
 * Obtem a altura de uma 'arvore recursivamente.
 *
 * Parameters:
 *     p_node: Root da arvore.
 */
int tree_height_recursive( struct node_t* p_node );

/*
 * Funcao auxiliar para remover um node de uma arvore.
 *
 * Parameters:
 *    p_tree: Arvore onde o node se encontra.
 *    p_node: Node a remover.
 *    p_key: Chave do node a remover.
 */
struct node_t* tree_del_node( struct tree_t* p_tree, struct node_t* p_node, char* p_key );


/*
 * Percorre a 'arvore inorder e insere as chaves dos nodes num array.
 *
 * Parameters:
 *  p_node: Node a partir do qual se inicia a percorrer a arvore.
 *  pp_keys: Array onde se vao inserir as chaves.
 *  p_index: Indice do array onde se vai inserir a proxima chave.
 */
void inorder_append_keys( struct node_t *p_node, char **pp_keys, int *p_index );

/*
 * Percorre a 'arvore inorder e insere os valores dos nodes num array.
 *
 * Parameters:
 *      p_node: Node a partir do qual se inicia a percorrer a arvore.
 *      pp_values: Array onde se vao inserir os valores.
 *      p_index: Indice do array onde se vai inserir o proximo valor.
 */
void inorder_append_values( struct node_t *p_node, void **pp_values, int *p_index );

#endif