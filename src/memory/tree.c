#include <clod/btree.h>
#include <clod/string.h>
#include <clod/stream/stream.h>
#include "debug.h"
#include <stdint.h>
#include <string.h>

#define NODE_SIZE 512

static void node_clear(struct clod_btree_node *node) {
    node->is_leaf = false;
    node->count = 0;
    node->parent = NULL;
    for (int i = 0; i < CLOD_BTREE_ORDER; i++) {
        node->u.data.children[i] = NULL;
    }
    for (int i = 0; i < CLOD_BTREE_ORDER - 1; i++) {
        node->u.keys[i] = 0;
        node->u.data.values[i] = 0;
    }
}

static struct clod_btree_node *node_alloc(struct clod_btree *tree) {
    if (!tree->allocator) return NULL;
    void *ptr = tree->allocator->allocate(tree->allocator, NODE_SIZE);
    if (ptr == NULL || ptr == (void *)1) return NULL;
    tree->alloc_cost += NODE_SIZE;
    return (struct clod_btree_node *)ptr;
}

static int find_key_index(const struct clod_btree_node *node, uint64_t key) {
    int i = 0;
    while (i < node->count && node->u.keys[i] < key) i++;
    return i;
}

static bool split_child(struct clod_btree *tree, struct clod_btree_node *parent, int idx) {
    struct clod_btree_node *child = parent->u.data.children[idx];
    if (!child || child->count < CLOD_BTREE_ORDER - 1) return true;

    struct clod_btree_node *new = node_alloc(tree);
    if (!new) return false;

    int t = CLOD_BTREE_MIN_KEYS;
    new->is_leaf = child->is_leaf;
    new->parent = parent;
    
    // B+ tree split: child keeps first t keys, new gets last t keys
    // Child has 2t keys (0 to 2t-1), t=10, so 20 keys
    // Child keeps 0-9 (10 keys), new gets 10-19 (10 keys)
    // Promote key 10 (first of new) to parent
    new->count = t;
    child->count = t;

    if (child->is_leaf) {
        for (int i = 0; i < t; i++) {
            new->u.keys[i] = child->u.keys[t + i];
            new->u.data.values[i] = child->u.data.values[t + i];
        }
    } else {
        for (int i = 0; i < t; i++) {
            new->u.keys[i] = child->u.keys[t + i];
            new->u.data.children[i] = child->u.data.children[t + i];
            if (child->u.data.children[t + i]) {
                child->u.data.children[t + i]->parent = new;
            }
        }
        new->u.data.children[t] = child->u.data.children[2 * t];
        if (new->u.data.children[t]) {
            new->u.data.children[t]->parent = new;
        }
    }
    
    uint64_t promoted_key = new->u.keys[0];

    int pos = find_key_index(parent, promoted_key);
    for (int i = parent->count; i > pos; i--) {
        parent->u.keys[i] = parent->u.keys[i - 1];
        if (!parent->is_leaf) {
            parent->u.data.children[i + 1] = parent->u.data.children[i];
        }
    }
    if (!parent->is_leaf) {
        parent->u.data.children[pos + 1] = parent->u.data.children[pos];
    }

    parent->u.keys[pos] = promoted_key;
    if (!parent->is_leaf) {
        parent->u.data.children[pos] = child;
        parent->u.data.children[pos + 1] = new;
    }
    parent->count++;
    
    return true;
}

static struct clod_btree_node *push_split(struct clod_btree *tree, struct clod_btree_node *node, uint64_t key) {
    while (!node->is_leaf) {
        int idx = find_key_index(node, key);
        
        if (node->u.data.children[idx]->count >= CLOD_BTREE_ORDER - 1) {
            if (!split_child(tree, node, idx)) {
                return NULL;
            }
            if (key > node->u.keys[idx]) {
                idx++;
            }
        }
        
        node = node->u.data.children[idx];
    }
    
    return node;
}

uint64_t *clod_btree_insert(struct clod_btree *tree, uint64_t key) {
    tree->alloc_cost = 0;
    
    if (!tree->root) {
        tree->root = node_alloc(tree);
        if (!tree->root) return NULL;
        node_clear(tree->root);
        tree->root->is_leaf = true;
        tree->root->count = 1;
        tree->root->u.keys[0] = key;
        tree->count = 1;
        return &tree->root->u.data.values[0];
    }
    
    if (tree->root->count >= CLOD_BTREE_ORDER - 1) {
        struct clod_btree_node *new_root = node_alloc(tree);
        if (!new_root) return NULL;
        node_clear(new_root);
        new_root->is_leaf = false;
        new_root->u.data.children[0] = tree->root;
        tree->root->parent = new_root;
        tree->root = new_root;
        
        if (!split_child(tree, tree->root, 0)) {
            return NULL;
        }
    }
    
    struct clod_btree_node *leaf = push_split(tree, tree->root, key);
    if (!leaf) return NULL;
    
    int i = leaf->count - 1;
    while (i >= 0 && leaf->u.keys[i] > key) {
        leaf->u.keys[i + 1] = leaf->u.keys[i];
        leaf->u.data.values[i + 1] = leaf->u.data.values[i];
        i--;
    }
    
    if (i >= 0 && leaf->u.keys[i] == key) {
        return &leaf->u.data.values[i];
    }
    
    leaf->u.keys[i + 1] = key;
    leaf->count++;
    tree->count++;
    
    return &leaf->u.data.values[i + 1];
}

bool clod_btree_get(const struct clod_btree *tree, uint64_t key, uint64_t *value) {
    if (!tree->root) return false;

    struct clod_btree_node *node = (struct clod_btree_node *)tree->root;
    while (node && !node->is_leaf) {
        int i = find_key_index(node, key);
        if (i < node->count && node->u.keys[i] == key) {
            node = node->u.data.children[i + 1];
        } else {
            node = node->u.data.children[i];
        }
    }
    if (!node || !node->is_leaf) return false;
    
    int i = find_key_index(node, key);
    if (i < node->count && node->u.keys[i] == key) {
        if (value) *value = node->u.data.values[i];
        return true;
    }
    return false;
}

static void erase_from_leaf(struct clod_btree_node *node, int idx) {
    for (int i = idx; i < node->count - 1; i++) {
        node->u.keys[i] = node->u.keys[i + 1];
        node->u.data.values[i] = node->u.data.values[i + 1];
    }
    node->count--;
}

// Delete that just removes the key from the leaf without any rebalancing
static bool simple_delete_from_tree(struct clod_btree *tree, struct clod_btree_node *node, uint64_t key, uint64_t *value_out) {
    (void)tree;  // Not used in simplified delete
    if (!node) return false;
    
    // For B+ tree, always go to leaf
    while (!node->is_leaf) {
        int idx = find_key_index(node, key);
        if (idx < node->count && node->u.keys[idx] == key) {
            node = node->u.data.children[idx + 1];
        } else {
            node = node->u.data.children[idx];
        }
    }
    
    // Now we're at a leaf
    int idx = find_key_index(node, key);
    if (idx < node->count && node->u.keys[idx] == key) {
        if (value_out) *value_out = node->u.data.values[idx];
        erase_from_leaf(node, idx);
        return true;
    }
    return false;
}

bool clod_btree_delete(struct clod_btree *tree, uint64_t key, uint64_t *value) {
    if (!tree->root) return false;

    bool result = simple_delete_from_tree(tree, tree->root, key, value);
    if (result) {
        tree->count--;
        // Don't rebalance - just leave the tree with potentially underfull nodes
        // This is acceptable for the memory allocator use case
    }
    return result;
}

bool clod_btree_predecessor(const struct clod_btree *tree, uint64_t key, uint64_t *out_key, uint64_t *out_value) {
    if (!tree->root) return false;

    const struct clod_btree_node *node = tree->root;
    uint64_t pred_key = 0, pred_value = 0;
    bool found = false;

    while (node) {
        int i = find_key_index(node, key);

        if (i > 0) {
            pred_key = node->u.keys[i - 1];
            pred_value = node->u.data.values[i - 1];
            found = true;
            if (node->is_leaf) break;
            node = node->u.data.children[i];
        } else {
            if (node->is_leaf) break;
            node = node->u.data.children[0];
        }
    }

    if (!found) return false;
    if (out_key) *out_key = pred_key;
    if (out_value) *out_value = pred_value;
    return true;
}

bool clod_btree_iter_next(struct clod_btree *tree, clod_btree_iter *iter, uint64_t *key, uint64_t *value) {
    if (!tree->root) return false;

    if (!iter->node) {
        iter->node = tree->root;
        iter->index = 0;
        iter->child_index = 0;

        while (!iter->node->is_leaf && iter->node->u.data.children[iter->index]) {
            iter->node = iter->node->u.data.children[iter->index];
            iter->index = 0;
            iter->child_index = 0;
        }

        if (iter->index >= iter->node->count) {
            iter->node = NULL;
            iter->index = -1;
            iter->child_index = -1;
            return false;
        }

        if (key) *key = iter->node->u.keys[iter->index];
        if (value) *value = iter->node->u.data.values[iter->index];
        iter->index++;
        return true;
    }

    if (iter->index < iter->node->count) {
        if (key) *key = iter->node->u.keys[iter->index];
        if (value) *value = iter->node->u.data.values[iter->index];
        iter->index++;
        return true;
    }

    struct clod_btree_node *current = iter->node;

    while (current) {
        struct clod_btree_node *parent = current->parent;
        if (!parent) {
            iter->node = NULL;
            iter->index = -1;
            iter->child_index = -1;
            return false;
        }

        int p_idx = 0;
        for (int i = 0; i <= parent->count; i++) {
            if (parent->u.data.children[i] == current) {
                p_idx = i;
                break;
            }
        }

        if (p_idx + 1 <= parent->count) {
            iter->node = parent->u.data.children[p_idx + 1];
            iter->child_index = p_idx + 1;
            iter->index = 0;
            while (!iter->node->is_leaf) {
                iter->node = iter->node->u.data.children[iter->index];
                iter->index = 0;
            }
            iter->child_index = 0;
            if (iter->index >= iter->node->count) {
                current = parent;
                continue;
            }
            if (key) *key = iter->node->u.keys[iter->index];
            if (value) *value = iter->node->u.data.values[iter->index];
            iter->index++;
            return true;
        }

        current = parent;
    }

    iter->node = NULL;
    iter->index = -1;
    iter->child_index = -1;
    return false;
}

size_t clod_btree_get_alloc_cost(const struct clod_btree *tree) {
    return tree->alloc_cost;
}

void clod_btree_create(struct clod_btree *tree, const struct clod_tree_opts *opts) {
    tree->root = NULL;
    tree->count = 0;
    tree->allocator = opts ? opts->allocator : NULL;
    tree->self_funding = opts ? opts->self_funding : false;
    tree->alloc_cost = 0;
    tree->alloc_user = opts ? opts->alloc_user : NULL;
}

static void destroy_node(struct clod_btree *tree, struct clod_btree_node *node) {
    if (!node) return;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->count; i++) {
            destroy_node(tree, node->u.data.children[i]);
        }
    }
    tree->allocator->free(tree->allocator, node);
}

void clod_btree_destroy(struct clod_btree *tree) {
    if (!tree->root) return;

    if (tree->self_funding) {
        tree->root = NULL;
        tree->count = 0;
        return;
    }

    destroy_node(tree, tree->root);
    tree->root = NULL;
    tree->count = 0;
}
