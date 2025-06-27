#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORDER 5
#define MAX_NAME 100
#define DATA_FILE "expense_data.txt"
#define CATEGORY_COUNT 5
typedef enum {
    RENT,
    UTILITY,
    GROCERY,
    STATIONARY,
    LEISURE
} ExpenseCategory;

const char* category_names[] = {"Rent", "Utility", "Grocery", "Stationary", "Leisure"};

typedef struct ExpenseNode {
    int expense_id;
    float amount;
    int date;  // ddmmyyyy format
    ExpenseCategory category;
} ExpenseNode;

typedef struct BPlusTreeNode {
    int is_leaf;
    int num_keys;
    int keys[ORDER - 1];
    
    union {
        struct BPlusTreeNode* children[ORDER];
        struct {
            void* data_ptrs[ORDER - 1];
            struct BPlusTreeNode* next;
        };
    };
} BPlusTreeNode;

typedef struct BPlusTree {
    BPlusTreeNode* root;
    int data_type;
} BPlusTree;

typedef struct IndividualNode {
    int individual_id;
    char username[MAX_NAME];
    float income;
    BPlusTree* expenses;
} IndividualNode;

typedef struct FamilyNode {
    int family_id;
    char family_name[MAX_NAME];
    float total_income;
    float monthly_expense;
    BPlusTree* members;
} FamilyNode;

int find_key_index(BPlusTreeNode* node, int key) {
    int index = -1;
    if (node) {
        for (int i = 0; i < node->num_keys; i++) {
            if (node->keys[i] == key) {
                index = i;
                break;
            }
        }
    }
    return index;
}

ExpenseNode* create_expense(int id, float amount, int date, ExpenseCategory cat) {
    ExpenseNode* new = (ExpenseNode*)malloc(sizeof(ExpenseNode));
    if (new) {
        new->expense_id = id;
        new->amount = amount;
        new->date = date;
        new->category = cat;
    }
    return new;
}

IndividualNode* create_individual(int id, const char* name, float income) {
    IndividualNode* new = (IndividualNode*)malloc(sizeof(IndividualNode));
    if (new) {
        new->individual_id = id;
        strncpy(new->username, name, MAX_NAME);
        new->income = income;
        new->expenses = NULL;
    }
    return new;
}

FamilyNode* create_family(int id, const char* name) {
    FamilyNode* new = (FamilyNode*)malloc(sizeof(FamilyNode));
    if (new) {
        new->family_id = id;
        strncpy(new->family_name, name, MAX_NAME);
        new->total_income = 0;
        new->monthly_expense = 0;
        new->members = NULL;
    }
    return new;
}

BPlusTreeNode* create_node(int is_leaf) {
    BPlusTreeNode* node = (BPlusTreeNode*)malloc(sizeof(BPlusTreeNode));
    if (node) {
        node->is_leaf = is_leaf;
        node->num_keys = 0;
        
        if(is_leaf) {
            for(int i = 0; i < ORDER-1; i++) {
                node->data_ptrs[i] = NULL;
            }
            node->next = NULL;
        } else {
            for(int i = 0; i < ORDER; i++) {
                node->children[i] = NULL;
            }
        }
    }
    return node;
}

BPlusTree* create_tree(int type) {
    BPlusTree* tree = (BPlusTree*)malloc(sizeof(BPlusTree));
    if (tree) {
        tree->root = create_node(1);
        tree->data_type = type;
    }
    return tree;
}

int get_first_key(BPlusTreeNode* node) {
    int key = 0;
    while(node && !node->is_leaf) {
        node = node->children[0];
    }
    if (node && node->num_keys > 0) {
        key = node->keys[0];
    }
    return key;
}

BPlusTreeNode* split_leaf(BPlusTreeNode* leaf) {
    BPlusTreeNode* new_leaf = NULL;
    if (leaf) {
        new_leaf = create_node(1);
        if (new_leaf) {
            int split_pos = leaf->num_keys / 2;
            
            for(int i = split_pos, j = 0; i < leaf->num_keys; i++, j++) {
                new_leaf->keys[j] = leaf->keys[i];
                new_leaf->data_ptrs[j] = leaf->data_ptrs[i];
                new_leaf->num_keys++;
            }
            
            leaf->num_keys = split_pos;
            new_leaf->next = leaf->next;
            leaf->next = new_leaf;
        }
    }
    return new_leaf;
}

BPlusTreeNode* split_internal(BPlusTreeNode* internal) {
    BPlusTreeNode* new_internal = NULL;
    if (internal) {
        new_internal = create_node(0);
        if (new_internal) {
            int split_pos = internal->num_keys / 2;
            
            for(int i = split_pos+1, j = 0; i < internal->num_keys; i++, j++) {
                new_internal->keys[j] = internal->keys[i];
                new_internal->children[j] = internal->children[i];
                new_internal->num_keys++;
            }
            new_internal->children[new_internal->num_keys] = internal->children[internal->num_keys];
            
            internal->num_keys = split_pos;
        }
    }
    return new_internal;
}

BPlusTreeNode* insert_into_leaf(BPlusTreeNode* leaf, int key, void* value) {
    BPlusTreeNode* new_leaf = NULL;
    if (leaf) {
        int pos = 0;
        while(pos < leaf->num_keys && key > leaf->keys[pos]) {
            pos++;
        }
        
        for(int i = leaf->num_keys; i > pos; i--) {
            leaf->keys[i] = leaf->keys[i-1];
            leaf->data_ptrs[i] = leaf->data_ptrs[i-1];
        }
        
        leaf->keys[pos] = key;
        leaf->data_ptrs[pos] = value;
        leaf->num_keys++;
        
        if(leaf->num_keys >= ORDER) {
            new_leaf = split_leaf(leaf);
        }
    }
    return new_leaf;
}

BPlusTreeNode* insert_into_internal(BPlusTreeNode* parent, int pos, BPlusTreeNode* child) {
    BPlusTreeNode* new_internal = NULL;
    if (parent && child) {
        for(int i = parent->num_keys; i > pos; i--) {
            parent->keys[i] = parent->keys[i-1];
            parent->children[i+1] = parent->children[i];
        }
        
        parent->keys[pos] = get_first_key(child);
        parent->children[pos+1] = child;
        parent->num_keys++;
        
        if(parent->num_keys >= ORDER) {
            new_internal = split_internal(parent);
        }
    }
    return new_internal;
}

BPlusTreeNode* insert_recursive(BPlusTreeNode* node, int key, void* value) {
    BPlusTreeNode* new_child = NULL;
    if (node) {
        int pos = 0;
        while(pos < node->num_keys && key > node->keys[pos]) {
            pos++;
        }
        
        if(node->is_leaf) {
            new_child = insert_into_leaf(node, key, value);
        } else {
            BPlusTreeNode* temp = insert_recursive(node->children[pos], key, value);
            if(temp) {
                new_child = insert_into_internal(node, pos, temp);
            }
        }
    }
    return new_child;
}

void insert(BPlusTree* tree, int key, void* value) {
    if(tree) {
        if(!tree->root) {
            tree->root = create_node(1);
        }
        
        BPlusTreeNode* new_child = insert_recursive(tree->root, key, value);
        if(new_child) {
            BPlusTreeNode* new_root = create_node(0);
            if (new_root) {
                new_root->children[0] = tree->root;
                new_root->children[1] = new_child;
                new_root->keys[0] = get_first_key(new_child);
                new_root->num_keys = 1;
                tree->root = new_root;
            }
        }
    }
}

FamilyNode* search_family(BPlusTree* family_tree, int family_id) {
    FamilyNode* result = NULL;
    if(family_tree && family_tree->root) {
        BPlusTreeNode* current = family_tree->root;
        int found = 0;
        
        while(!found) {
            int i = 0;
            while(i < current->num_keys && family_id > current->keys[i]) {
                i++;
            }
            
            if(i < current->num_keys && family_id == current->keys[i]) {
                if(current->is_leaf) {
                    result = (FamilyNode*)current->data_ptrs[i];
                    found = 1;
                }
                i++;
            }
            
            if(!found) {
                if(current->is_leaf) {
                    break;
                } else {
                    current = current->children[i];
                }
            }
        }
    }
    return result;
}

IndividualNode* search_individual(BPlusTree* member_tree, int individual_id) {
    IndividualNode* result = NULL;
    if(member_tree && member_tree->root) {
        BPlusTreeNode* current = member_tree->root;
        int found = 0;
        
        while(!found) {
            int i = 0;
            while(i < current->num_keys && individual_id > current->keys[i]) {
                i++;
            }
            
            if(i < current->num_keys && individual_id == current->keys[i]) {
                if(current->is_leaf) {
                    result = (IndividualNode*)current->data_ptrs[i];
                    found = 1;
                }
                i++;
            }
            
            if(!found) {
                if(current->is_leaf) {
                    break;
                } else {
                    current = current->children[i];
                }
            }
        }
    }
    return result;
}

ExpenseNode* search_expense(BPlusTree* expense_tree, int expense_id) {
    ExpenseNode* result = NULL;
    if(expense_tree && expense_tree->root) {
        BPlusTreeNode* current = expense_tree->root;
        int found = 0;
        
        while(!found) {
            int i = 0;
            while(i < current->num_keys && expense_id > current->keys[i]) {
                i++;
            }
            
            if(i < current->num_keys && expense_id == current->keys[i]) {
                if(current->is_leaf) {
                    result = (ExpenseNode*)current->data_ptrs[i];
                    found = 1;
                }
                i++;
            }
            
            if(!found) {
                if(!current->is_leaf) {
                    break;
                } else {
                    current = current->children[i];
                }
            }
        }
    }
    return result;
}

int get_predecessor(BPlusTreeNode* node) {
    while (!node->is_leaf) {
        node = node->children[node->num_keys];
    }
    return node->keys[node->num_keys - 1];
}

int get_successor(BPlusTreeNode* node) {
    while (!node->is_leaf) {
        node = node->children[0];
    }
    return node->keys[0];
}

void remove_from_leaf(BPlusTreeNode* node, int index) {
    for (int i = index + 1; i < node->num_keys; i++) {
        node->keys[i - 1] = node->keys[i];
        node->data_ptrs[i - 1] = node->data_ptrs[i];
    }
    node->num_keys--;
}

void remove_from_internal(BPlusTreeNode* node, int index) {
    for (int i = index + 1; i < node->num_keys; i++) {
        node->keys[i - 1] = node->keys[i];
        node->children[i] = node->children[i + 1];
    }
    node->num_keys--;
}

void borrow_from_left(BPlusTreeNode* parent, int child_index, 
                    BPlusTreeNode* child, BPlusTreeNode* left_sibling) {
    for (int i = child->num_keys; i > 0; i--) {
        child->keys[i] = child->keys[i - 1];
    }
    
    if (!child->is_leaf) {
        for (int i = child->num_keys + 1; i > 0; i--) {
            child->children[i] = child->children[i - 1];
        }
    }
    
    child->keys[0] = parent->keys[child_index - 1];
    if (!child->is_leaf) {
        child->children[0] = left_sibling->children[left_sibling->num_keys];
    } else {
        child->data_ptrs[0] = left_sibling->data_ptrs[left_sibling->num_keys - 1];
    }
    
    parent->keys[child_index - 1] = left_sibling->keys[left_sibling->num_keys - 1];
    
    child->num_keys++;
    left_sibling->num_keys--;
}

void borrow_from_right(BPlusTreeNode* parent, int child_index,
                     BPlusTreeNode* child, BPlusTreeNode* right_sibling) {
    child->keys[child->num_keys] = parent->keys[child_index];
    if (!child->is_leaf) {
        child->children[child->num_keys + 1] = right_sibling->children[0];
    } else {
        child->data_ptrs[child->num_keys] = right_sibling->data_ptrs[0];
    }
    
    parent->keys[child_index] = right_sibling->keys[0];
    
    for (int i = 1; i < right_sibling->num_keys; i++) {
        right_sibling->keys[i - 1] = right_sibling->keys[i];
    }
    
    if (!right_sibling->is_leaf) {
        for (int i = 1; i <= right_sibling->num_keys; i++) {
            right_sibling->children[i - 1] = right_sibling->children[i];
        }
    } else {
        for (int i = 1; i < right_sibling->num_keys; i++) {
            right_sibling->data_ptrs[i - 1] = right_sibling->data_ptrs[i];
        }
    }
    
    child->num_keys++;
    right_sibling->num_keys--;
}

void merge_nodes(BPlusTreeNode* parent, int child_index,
                BPlusTreeNode* left, BPlusTreeNode* right) {
    left->keys[left->num_keys] = parent->keys[child_index];
    
    for (int i = 0; i < right->num_keys; i++) {
        left->keys[left->num_keys + 1 + i] = right->keys[i];
    }
    
    if (!left->is_leaf) {
        for (int i = 0; i <= right->num_keys; i++) {
            left->children[left->num_keys + 1 + i] = right->children[i];
        }
    } else {
        for (int i = 0; i < right->num_keys; i++) {
            left->data_ptrs[left->num_keys + 1 + i] = right->data_ptrs[i];
        }
        left->next = right->next;
    }
    
    left->num_keys += right->num_keys + 1;
    remove_from_internal(parent, child_index);
    free(right);
}

void fix_underflow(BPlusTree* tree, BPlusTreeNode* parent, int child_index, BPlusTreeNode* child) {
    if (!tree || !parent || !child) return;
    
    int fixed = 0;
    
    if (child_index > 0) {
        BPlusTreeNode* left = parent->children[child_index - 1];
        if (left->num_keys > (ORDER - 1) / 2) {
            borrow_from_left(parent, child_index, child, left);
            fixed = 1;
        }
    }
    
    if (!fixed && child_index < parent->num_keys) {
        BPlusTreeNode* right = parent->children[child_index + 1];
        if (right->num_keys > (ORDER - 1) / 2) {
            borrow_from_right(parent, child_index, child, right);
            fixed = 1;
        }
    }
    
    if (!fixed) {
        if (child_index > 0) {
            merge_nodes(parent, child_index - 1, parent->children[child_index - 1], child);
        } else {
            merge_nodes(parent, child_index, child, parent->children[child_index + 1]);
        }
        
        if (parent == tree->root && parent->num_keys == 0) {
            free(tree->root);
            tree->root = (child_index > 0) ? parent->children[child_index - 1] 
                                         : parent->children[child_index];
        }
    }
}

int delete_from_tree(BPlusTree* tree, int key) {
    int result = 0;
    
    if (tree && tree->root) {
        BPlusTreeNode* current = tree->root;
        BPlusTreeNode* parent = NULL;
        int child_index = 0;
        int found = 0;
        int index = -1;

        while (!found && current) {
            index = find_key_index(current, key);
            if (index != -1 && current->is_leaf) {
                found = 1;
            } else {
                parent = current;
                child_index = 0;
                while (child_index < current->num_keys && key > current->keys[child_index]) {
                    child_index++;
                }
                current = current->children[child_index];
            }
        }

        if (found) {
            if (current->is_leaf) {
                remove_from_leaf(current, index);
                
                if (tree->data_type == 2) {
                    free(current->data_ptrs[index]);
                }
                
                if (current != tree->root && current->num_keys < (ORDER - 1) / 2) {
                    fix_underflow(tree, parent, child_index, current);
                }
                result = 1;
            } else {
                if (current->children[index]->num_keys > (ORDER - 1) / 2) {
                    int predecessor = get_predecessor(current->children[index]);
                    result = delete_from_tree(tree, predecessor);
                    current->keys[index] = predecessor;
                } 
                else if (current->children[index + 1]->num_keys > (ORDER - 1) / 2) {
                    int successor = get_successor(current->children[index + 1]);
                    result = delete_from_tree(tree, successor);
                    current->keys[index] = successor;
                } 
                else {
                    merge_nodes(current, index, current->children[index], current->children[index + 1]);
                    result = delete_from_tree(tree, key);
                }
            }
        }
    }
    return result;
}

int delete_expense(IndividualNode* individual, int expense_id) {
    int success = 0;
    if (individual && individual->expenses) {
        ExpenseNode* expense = search_expense(individual->expenses, expense_id);
        if (expense) {
            success = delete_from_tree(individual->expenses, expense_id);
            if (success) {
                free(expense);
            }
        }
    }
    return success;
}

void update_expense(IndividualNode* Individual, int expense_id)
{
    if( Individual && Individual->expenses)
    {
        ExpenseNode* expense=search_expense(Individual->expenses,expense_id);
        if(expense==NULL)
        {
            printf("expense not found");
        }
        else
        {
            int choice;
            printf("what do you want to change\n 1.change the date \n 2.change the amount \n 3.change the category ");
            scanf("%d",&choice);
            switch(choice)
            {
                case 1:
                {
                    int new_date;
                    printf("the old date is %d",expense->date);
                    printf("enter the new date");
                    scanf("%d",&new_date);
                    expense->date=new_date;
                    printf("expense date is updated");
                    break;
                }
                case 2:
                {
                    float new_amount;
                    printf("the old amount is %f",expense->amount);
                    printf("enter the new amount");
                    scanf("%f",&new_amount);
                    expense->amount=new_amount;
                    printf("expense amount has been updated");
                    break;
                }
                case 3:
                {
                    ExpenseCategory new_category;
                    printf("the old category is %s", category_names[expense->category]);
                    printf("enter the new category\n 0.rent 1.Utility 2.Grocery 3.Stationary 4.Leisure");
                    scanf("%d",&new_category);
                    expense->category=new_category;
                    printf("Category has been successfully updated");
                    break;
                }
                default:
                {
                    printf("choose a valid option");
                }
            }
        }
    }
}

int delete_individual(FamilyNode* family, int individual_id) {
    int success = 0;
    if (family && family->members) {
        IndividualNode* individual = search_individual(family->members, individual_id);
        if (individual) {
            if (individual->expenses) {
                BPlusTreeNode* leaf = individual->expenses->root;
                while (leaf && !leaf->is_leaf) {
                    leaf = leaf->children[0];
                }
                
                while (leaf) {
                    for (int i = 0; i < leaf->num_keys; i++) {
                        free(leaf->data_ptrs[i]);
                    }
                    BPlusTreeNode* next = leaf->next;
                    free(leaf);
                    leaf = next;
                }
                free(individual->expenses);
            }
            
            success = delete_from_tree(family->members, individual_id);
            if (success) {
                family->total_income -= individual->income;
                free(individual);
            }
        }
    }
    return success;
}


void update_individual(FamilyNode* family, int individual_id)
{
    if (family && family->members) 
    {
        IndividualNode* individual = search_individual(family->members, individual_id);
        if(individual==NULL)
        {
            printf("individual not found");
        }
        else
        {
            int choice;
            printf("what do you want to update in individual\n 1.change individual name \n 2.change individual income ");
            scanf("%d",&choice);
            switch(choice)
            {
                case 1:
                {
                    char new_name[MAX_NAME];
                    printf("current name of individual is %s", individual->username);
                    printf("enter the new name of individual");
                    scanf("%s",new_name);
                    strncpy(individual->username,new_name,MAX_NAME);
                    printf("successfully updated the name of individual");
                    break;
                }
                case 2:
                {
                    float new_income;
                    printf("current income of individual is %f",individual->income);
                    printf("enter new income");
                    scanf("%f",&new_income);
                    individual->income=new_income;
                    printf("successfully updated the income of individual");
                    break;
                }

                default:
                {
                    printf("invalid choice , select valid choice");
                }
            }
        }
    }
}

int delete_family(BPlusTree* db, int family_id) {
    int success = 0;
    if (db) {
        FamilyNode* family = search_family(db, family_id);
        if (family) {
            if (family->members) {
                BPlusTreeNode* leaf = family->members->root;
                while (leaf && !leaf->is_leaf) {
                    leaf = leaf->children[0];
                }
                
                while (leaf) {
                    for (int i = 0; i < leaf->num_keys; i++) {
                        IndividualNode* individual = (IndividualNode*)leaf->data_ptrs[i];
                        if (individual) {
                            if (individual->expenses) {
                                BPlusTreeNode* exp_leaf = individual->expenses->root;
                                while (exp_leaf && !exp_leaf->is_leaf) {
                                    exp_leaf = exp_leaf->children[0];
                                }
                                
                                while (exp_leaf) {
                                    for (int j = 0; j < exp_leaf->num_keys; j++) {
                                        free(exp_leaf->data_ptrs[j]);
                                    }
                                    BPlusTreeNode* next = exp_leaf->next;
                                    free(exp_leaf);
                                    exp_leaf = next;
                                }
                                free(individual->expenses);
                            }
                            free(individual);
                        }
                    }
                    BPlusTreeNode* next = leaf->next;
                    free(leaf);
                    leaf = next;
                }
                free(family->members);
            }
            
            success = delete_from_tree(db, family_id);
            if (success) {
                free(family);
            }
        }
    }
    return success;
}

void update_family(BPlusTree* db, int family_id) 
{
    if (db) 
    {
        FamilyNode* family = search_family(db, family_id);
        if(family==NULL)
        {
            printf("family not found");
        }
        else
        {
            char new_fam_name[MAX_NAME];
            printf("update family name");
            printf("current family name is %s", family->family_name);
            printf("new family name is ");
            scanf("%s", new_fam_name);
            strncpy(family->family_name, new_fam_name,MAX_NAME);
            printf("successfully updated new name");
        }
    }
}

void get_categorical_expense(FamilyNode* family, const char* category_name) {
    if (!family || !family->members || !family->members->root) {
        printf("No family members found!\n");
        return;
    }

    ExpenseCategory category = -1;
    for (int i = 0; i < CATEGORY_COUNT; i++) {
        if (strcmp(category_name, category_names[i]) == 0) {
            category = i;
            break;
        }
    }

    if (category == -1) {
        printf("Invalid category!\n");
        return;
    }

    float family_total = 0;
    int member_count = 0;
    float* amounts = NULL;
    int* ids = NULL;
    char** names = NULL;

    BPlusTreeNode* count_node = family->members->root;
    while (!count_node->is_leaf) {
        count_node = count_node->children[0];
    }

    while (count_node != NULL) {
        member_count += count_node->num_keys;
        count_node = count_node->next;
    }

    if (member_count == 0) {
        printf("No members found in this family!\n");
        return;
    }

    amounts = (float*)malloc(member_count * sizeof(float));
    ids = (int*)malloc(member_count * sizeof(int));
    names = (char**)malloc(member_count * sizeof(char*));
    for (int i = 0; i < member_count; i++) {
        names[i] = (char*)malloc(MAX_NAME * sizeof(char));
    }

    int current_member = 0;
    BPlusTreeNode* member_node = family->members->root;
    while (!member_node->is_leaf) {
        member_node = member_node->children[0];
    }

    while (member_node != NULL) {
        for (int i = 0; i < member_node->num_keys; i++) {
            IndividualNode* member = (IndividualNode*)member_node->data_ptrs[i];
            float member_total = 0;

            if (member->expenses && member->expenses->root) {
                BPlusTreeNode* expense_node = member->expenses->root;
                while (!expense_node->is_leaf) {
                    expense_node = expense_node->children[0];
                }

                while (expense_node != NULL) {
                    for (int j = 0; j < expense_node->num_keys; j++) {
                        ExpenseNode* expense = (ExpenseNode*)expense_node->data_ptrs[j];
                        if (expense->category == category) {
                            member_total += expense->amount;
                        }
                    }
                    expense_node = expense_node->next;
                }
            }

            ids[current_member] = member->individual_id;
            strcpy(names[current_member], member->username);
            amounts[current_member] = member_total;
            family_total += member_total;
            current_member++;
        }
        member_node = member_node->next;
    }

    for (int i = 0; i < member_count - 1; i++) {
        for (int j = 0; j < member_count - i - 1; j++) {
            if (amounts[j] < amounts[j + 1]) {
                float temp_amount = amounts[j];
                amounts[j] = amounts[j + 1];
                amounts[j + 1] = temp_amount;

                int temp_id = ids[j];
                ids[j] = ids[j + 1];
                ids[j + 1] = temp_id;

                char temp_name[MAX_NAME];
                strcpy(temp_name, names[j]);
                strcpy(names[j], names[j + 1]);
                strcpy(names[j + 1], temp_name);
            }
        }
    }

    printf("\nTotal %s expenses for family %s: %.2f\n", category_name, family->family_name, family_total);
    printf("Individual contributions:\n");
    for (int i = 0; i < member_count; i++) {
        if (amounts[i] > 0) {
            printf("%-20s (ID: %d): %.2f\n", names[i], ids[i], amounts[i]);
        }
    }

    for (int i = 0; i < member_count; i++) {
        free(names[i]);
    }
    free(names);
    free(ids);
    free(amounts);
}

void get_highest_expense_day(FamilyNode* family) {
    if (!family || !family->members || !family->members->root) {
        printf("No family members found!\n");
        return;
    }

    struct {
        int date;
        float amount;
    } date_totals[365]; 
    int date_count = 0;

    BPlusTreeNode* member_node = family->members->root;
    while (!member_node->is_leaf) {
        member_node = member_node->children[0];
    }

    while (member_node != NULL) {
        for (int i = 0; i < member_node->num_keys; i++) {
            IndividualNode* member = (IndividualNode*)member_node->data_ptrs[i];

            if (member->expenses && member->expenses->root) {
                BPlusTreeNode* expense_node = member->expenses->root;
                while (!expense_node->is_leaf) {
                    expense_node = expense_node->children[0];
                }

                while (expense_node != NULL) {
                    for (int j = 0; j < expense_node->num_keys; j++) {
                        ExpenseNode* expense = (ExpenseNode*)expense_node->data_ptrs[j];
                        int found = 0;
                        
                        for (int k = 0; k < date_count; k++) {
                            if (date_totals[k].date == expense->date) {
                                date_totals[k].amount += expense->amount;
                                found = 1;
                                break;
                            }
                        }

                        if (!found && date_count < 365) {
                            date_totals[date_count].date = expense->date;
                            date_totals[date_count].amount = expense->amount;
                            date_count++;
                        }
                    }
                    expense_node = expense_node->next;
                }
            }
        }
        member_node = member_node->next;
    }

    if (date_count == 0) {
        printf("No expenses found!\n");
        return;
    }

    int max_index = 0;
    for (int i = 1; i < date_count; i++) {
        if (date_totals[i].amount > date_totals[max_index].amount) {
            max_index = i;
        }
    }

    printf("Highest expense day for family %s:\n", family->family_name);
    printf("Date: %d, Total Amount: %.2f\n", date_totals[max_index].date, date_totals[max_index].amount);
}


void get_expense_in_period(FamilyNode* family, int date1, int date2) {
    if (!family || !family->members || !family->members->root) {
        printf("No family members found!\n");
        return;
    }

    // printf("\nExpenses between %d and %d for family %s:\n", date1, date2, family->family_name);

    BPlusTreeNode* member_node = family->members->root;
    while (!member_node->is_leaf) {
        member_node = member_node->children[0];
    }

    while (member_node != NULL) {
        for (int i = 0; i < member_node->num_keys; i++) {
            IndividualNode* member = (IndividualNode*)member_node->data_ptrs[i];
            // printf("\nMember: %s (ID: %d)\n", member->username, member->individual_id);

            if (member->expenses && member->expenses->root) {
                BPlusTreeNode* expense_node = member->expenses->root;
                while (!expense_node->is_leaf) {
                    expense_node = expense_node->children[0];
                }

                while (expense_node != NULL) {
                    for (int j = 0; j < expense_node->num_keys; j++) {
                        ExpenseNode* expense = (ExpenseNode*)expense_node->data_ptrs[j];
                        if (expense->date >= date1 && expense->date <= date2) {
                            printf(" family id: %d Member: %s (ID: %d)\n",family->family_id, member->username, member->individual_id);
                            printf("ID: %d, Amount: %.2f, Date: %d, Category: %s\n",
                                   expense->expense_id,
                                   expense->amount,
                                   expense->date,
                                   category_names[expense->category]);
                        }
                    }
                    expense_node = expense_node->next;
                }
            }
        }
        member_node = member_node->next;
    }
}

void get_expense_in_range(IndividualNode* individual, int expense_id1, int expense_id2) {
    if (!individual || !individual->expenses || !individual->expenses->root) {
        printf("No expenses found for this individual!\n");
        return;
    }

    printf("\nExpenses between ID %d and %d for %s:\n", 
           expense_id1, expense_id2, individual->username);

    BPlusTreeNode* expense_node = individual->expenses->root;
    while (!expense_node->is_leaf) {
        expense_node = expense_node->children[0];
    }

    while (expense_node != NULL) {
        for (int j = 0; j < expense_node->num_keys; j++) {
            ExpenseNode* expense = (ExpenseNode*)expense_node->data_ptrs[j];
            if (expense->expense_id >= expense_id1 && expense->expense_id <= expense_id2) {
                printf("ID: %d, Amount: %.2f, Date: %d, Category: %s\n",
                       expense->expense_id,
                       expense->amount,
                       expense->date,
                       category_names[expense->category]);
            }
        }
        expense_node = expense_node->next;
    }
}

void save_expense(FILE* fp, ExpenseNode* exp) {
    fprintf(fp, "EXPENSE %d %.2f %d %s\n", 
            exp->expense_id, exp->amount, exp->date, 
            category_names[exp->category]);
}

void save_individual(FILE* fp, IndividualNode* ind) {
    fprintf(fp, "INDIVIDUAL %d \"%s\" %.2f\n", 
            ind->individual_id, ind->username, ind->income);
    
    if(ind->expenses && ind->expenses->root) {
        fprintf(fp, "BEGIN_EXPENSES %d\n", ind->individual_id);
        BPlusTreeNode* node = ind->expenses->root;
        while(!node->is_leaf) node = node->children[0];
        
        while(node) {
            for(int i = 0; i < node->num_keys; i++) {
                save_expense(fp, node->data_ptrs[i]);
            }
            node = node->next;
        }
        fprintf(fp, "END_EXPENSES %d\n", ind->individual_id);
    }
}

void save_family(FILE* fp, FamilyNode* fam) {
    fprintf(fp, "FAMILY %d \"%s\" %.2f %.2f\n", 
            fam->family_id, fam->family_name, 
            fam->total_income, fam->monthly_expense);
    
    if(fam->members && fam->members->root) {
        fprintf(fp, "BEGIN_MEMBERS %d\n", fam->family_id);
        BPlusTreeNode* node = fam->members->root;
        while(!node->is_leaf) node = node->children[0];
        
        while(node) {
            for(int i = 0; i < node->num_keys; i++) {
                save_individual(fp, node->data_ptrs[i]);
            }
            node = node->next;
        }
        fprintf(fp, "END_MEMBERS %d\n", fam->family_id);
    }
}

void save_database(BPlusTree* db) {
    FILE* fp = fopen(DATA_FILE, "w");
    if(!fp) {
        perror("Failed to save database");
        return;
    }
    
    fprintf(fp, "DATABASE_VERSION 1.0\n");
    
    if(db->root) {
        BPlusTreeNode* node = db->root;
        while(!node->is_leaf) node = node->children[0];
        
        while(node) {
            for(int i = 0; i < node->num_keys; i++) {
                save_family(fp, node->data_ptrs[i]);
            }
            node = node->next;
        }
    }
    
    fclose(fp);
    printf("Database saved successfully\n");
}

BPlusTree* load_database() 
{
    FILE* fp = fopen(DATA_FILE, "r");
    if(!fp) {
        printf("Creating new database\n");
        return create_tree(0);
    }
    
    BPlusTree* db = create_tree(0);
    char line[256];
    FamilyNode* current_family = NULL;
    IndividualNode* current_individual = NULL;
    int loading_expenses = 0;
    int loading_members = 0;
    int current_id = 0;
    
    while(fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        
        if(strncmp(line, "FAMILY", 6) == 0) {
            int id;
            char name[MAX_NAME];
            float income, expense;
            sscanf(line, "FAMILY %d \"%[^\"]\" %f %f", &id, name, &income, &expense);
            current_family = create_family(id, name);
            current_family->total_income = income;
            current_family->monthly_expense = expense;
            current_family->members = create_tree(1);
            insert(db, id, current_family);
        }
        else if(strncmp(line, "INDIVIDUAL", 10) == 0) {
            int id;
            char name[MAX_NAME];
            float income;
            sscanf(line, "INDIVIDUAL %d \"%[^\"]\" %f", &id, name, &income);
            current_individual = create_individual(id, name, income);
            current_individual->expenses = create_tree(2);
            insert(current_family->members, id, current_individual);
        }
        else if(strncmp(line, "EXPENSE", 7) == 0) {
            int id, date;
            float amount;
            char category[20];
            sscanf(line, "EXPENSE %d %f %d %s", &id, &amount, &date, category);
            
            ExpenseCategory cat;
            for(int i = 0; i < CATEGORY_COUNT; i++) {
                if(strcmp(category, category_names[i]) == 0) {
                    cat = i;
                    break;
                }
            }
            
            ExpenseNode* exp = create_expense(id, amount, date, cat);
            insert(current_individual->expenses, id, exp);
        }
        else if(strncmp(line, "BEGIN_EXPENSES", 14) == 0) {
            sscanf(line, "BEGIN_EXPENSES %d", &current_id);
        }
        else if(strncmp(line, "END_EXPENSES", 12) == 0) {
            current_id = 0;
        }
    }
    
    fclose(fp);
    printf("Database loaded successfully\n");
    return db;
}


float calculate_individual_expenses(IndividualNode* individual) 
{
    float retval=0;
    if (individual && individual->expenses && individual->expenses->root)
    {

        float total = 0.0f;
        BPlusTreeNode* node = individual->expenses->root;
        
        while (!node->is_leaf)
        {
            node = node->children[0];
        }


        while (node != NULL) 
        {
            for (int i = 0; i < node->num_keys; i++) 
            {
                ExpenseNode* expense = (ExpenseNode*)node->data_ptrs[i];
                total += expense->amount;
            }
            node = node->next;
        }
        return total;
    }
}



void display_main_menu() {
    printf("\n=== Expense Tracking System ===\n");
    printf("1. Add Family\n");
    printf("2. Add Family Member\n");
    printf("3. Add Expense\n");
    printf("4. Delete Family\n");
    printf("5. Delete Member\n");
    printf("6. Delete Expense\n");
    printf("7. Search Records\n");
    printf("8.Show total expense by an individual\n");
    printf("9. Get Categorical Expense\n");
    printf("10. Get Highest Expense Day\n");
    printf("11. Get Expenses in Period\n");
    printf("12. Get Expenses in Range\n");
    printf("13. Exit\n");
    printf("Enter your choice: ");
}

int main() {
    BPlusTree* db = load_database();
    int choice = 0;
    int running = 1;
    
    while(running) {
        display_main_menu();
        scanf("%d", &choice);
        
        switch(choice) {
            case 1: { // Add Family
                int family_id;
                char name[MAX_NAME];
                printf("Enter family ID: ");
                scanf("%d", &family_id);
                printf("Enter family name: ");
                scanf("%s", name);
                
                if (search_family(db, family_id)) {
                    printf("Error: Family with ID %d already exists!\n", family_id);
                } else {
                    FamilyNode* family = create_family(family_id, name);
                    family->members = create_tree(1);
                    insert(db, family_id, family);
                    printf("Family added successfully!\n");
                }
                break;
            }
            
            case 2: { // Add Member
                int family_id, member_id;
                char name[MAX_NAME];
                float income;
                
                printf("Enter family ID: ");
                scanf("%d", &family_id);
                FamilyNode* family = search_family(db, family_id);
                
                if (!family) {
                    printf("Error: Family not found!\n");
                } else {
                    printf("Enter member ID: ");
                    scanf("%d", &member_id);
                    printf("Enter member name: ");
                    scanf("%s", name);
                    printf("Enter member income: ");
                    scanf("%f", &income);
                    
                    if (search_individual(family->members, member_id)) {
                        printf("Error: Member with ID %d already exists!\n", member_id);
                    } else {
                        IndividualNode* member = create_individual(member_id, name, income);
                        member->expenses = create_tree(2);
                        insert(family->members, member_id, member);
                        family->total_income += income;
                        printf("Member added successfully!\n");
                    }
                }
                break;
            }
            
            case 3: { // Add Expense
                int family_id, member_id, expense_id, category, date;
                float amount;
                
                printf("Enter family ID: ");
                scanf("%d", &family_id);
                FamilyNode* family = search_family(db, family_id);
                
                if (!family) {
                    printf("Error: Family not found!\n");
                } else {
                    printf("Enter member ID: ");
                    scanf("%d", &member_id);
                    IndividualNode* member = search_individual(family->members, member_id);
                    
                    if (!member) {
                        printf("Error: Member not found!\n");
                    } else {
                        printf("Enter expense ID: ");
                        scanf("%d", &expense_id);
                        printf("Enter amount: ");
                        scanf("%f", &amount);
                        printf("Enter date (ddmmyyyy): ");
                        scanf("%d", &date);
                        printf("Categories: 0-Rent, 1-Utility, 2-Grocery, 3-Stationary, 4-Leisure\n");
                        printf("Enter category: ");
                        scanf("%d", &category);

                        if (category < 0 || category >= CATEGORY_COUNT) {
                            printf("Error: Invalid category!\n");
                        } else if (search_expense(member->expenses, expense_id)) {
                            printf("Error: Expense with ID %d already exists!\n", expense_id);
                        } else {
                            ExpenseNode* expense = create_expense(expense_id, amount, date, category);
                            insert(member->expenses, expense_id, expense);
                            family->monthly_expense += amount;
                            printf("Expense added successfully!\n");
                        }
                    }
                }
                break;
            }
            
            case 4: { // Delete Family
                int family_id;
                printf("Enter family ID to delete: ");
                scanf("%d", &family_id);
                
                if (delete_family(db, family_id)) {
                    printf("Family with ID %d deleted successfully!\n", family_id);
                } else {
                    printf("Error: Failed to delete family or family not found!\n");
                }
                break;
            }
            
            case 5: { // Delete Member
                int family_id, member_id;
                printf("Enter family ID: ");
                scanf("%d", &family_id);
                FamilyNode* family = search_family(db, family_id);
                
                if (!family) {
                    printf("Error: Family not found!\n");
                } else {
                    printf("Enter member ID to delete: ");
                    scanf("%d", &member_id);
                    
                    if (delete_individual(family, member_id)) {
                        printf("Member with ID %d deleted successfully!\n", member_id);
                    } else {
                        printf("Error: Failed to delete member or member not found!\n");
                    }
                }
                break;
            }
            
            case 6: { // Delete Expense
                int family_id, member_id, expense_id;
                printf("Enter family ID: ");
                scanf("%d", &family_id);
                FamilyNode* family = search_family(db, family_id);
                
                if (!family) {
                    printf("Error: Family not found!\n");
                } else {
                    printf("Enter member ID: ");
                    scanf("%d", &member_id);
                    IndividualNode* member = search_individual(family->members, member_id);
                    
                    if (!member) {
                        printf("Error: Member not found!\n");
                    } else {
                        printf("Enter expense ID to delete: ");
                        scanf("%d", &expense_id);
                        
                        if (delete_expense(member, expense_id)) {
                            printf("Expense with ID %d deleted successfully!\n", expense_id);
                        } else {
                            printf("Error: Failed to delete expense or expense not found!\n");
                        }
                    }
                }
                break;
            }
            
            case 7: { // Search Records
                int family_id, member_id, expense_id;
                printf("Enter family ID to search: ");
                scanf("%d", &family_id);
                FamilyNode* family = search_family(db, family_id);
                
                if (!family) {
                    printf("Family not found!\n");
                } else {
                    printf("\nFamily Found:\n");
                    printf("ID: %d, Name: %s\n", family->family_id, family->family_name);
                    printf("Total Income: %.2f\n", family->total_income);
                    printf("Monthly Expenses: %.2f\n", family->monthly_expense);
                    
                    printf("\nEnter member ID to search (0 to skip): ");
                    scanf("%d", &member_id);
                    if (member_id != 0) {
                        IndividualNode* member = search_individual(family->members, member_id);
                        if (!member) {
                            printf("Member not found!\n");
                        } else {
                            printf("\nMember Found:\n");
                            printf("ID: %d, Name: %s\n", member->individual_id, member->username);
                            printf("Income: %.2f\n", member->income);
                            
                            printf("\nEnter expense ID to search (0 to skip): ");
                            scanf("%d", &expense_id);
                            if (expense_id != 0) {
                                ExpenseNode* expense = search_expense(member->expenses, expense_id);
                                if (!expense) {
                                    printf("Expense not found!\n");
                                } else {
                                    printf("\nExpense Found:\n");
                                    printf("ID: %d, Amount: %.2f, Date: %d\n", 
                                           expense->expense_id, expense->amount, expense->date);
                                    printf("Category: %s\n", category_names[expense->category]);
                                }
                            }
                        }
                    }
                }
                break;
            }
            
            case 8: {
                int family_id, individual_id;
                
                printf("Enter family ID: ");
                scanf("%d", &family_id);
                FamilyNode* family = search_family(db, family_id);
                if(!family) {
                    printf("Family not found!\n");
                    break;
                }
                
                printf("Enter member ID: ");
                scanf("%d", &individual_id);
                IndividualNode* member = search_individual(family->members, individual_id);
                if(!member) {
                    printf("Member not found in this family!\n");
                    break;
                }
                
                float total = calculate_individual_expenses(member);
                printf("\nTotal expenses for %s (ID: %d): %.2f\n", 
                       member->username, member->individual_id, total);
                
                // Optional: Show category breakdown
                if(member->expenses && member->expenses->root) {
                    float categories[CATEGORY_COUNT] = {0};
                    BPlusTreeNode* node = member->expenses->root;
                    while(!node->is_leaf) node = node->children[0];
                    
                    while(node) {
                        for(int i = 0; i < node->num_keys; i++) {
                            ExpenseNode* exp = node->data_ptrs[i];
                            categories[exp->category] += exp->amount;
                        }
                        node = node->next;
                    }
                    
                    printf("\nExpense Breakdown:\n");
                    for(int i = 0; i < CATEGORY_COUNT; i++) {
                        if(categories[i] > 0) {
                            printf("%-12s: %.2f\n", category_names[i], categories[i]);
                        }
                    }
                }
                break;
            }

            case 9: {//categorial expense
                int family_id;
                char category[20];
                
                printf("Enter family ID: ");
                scanf("%d", &family_id);
                FamilyNode* family = search_family(db, family_id);
                if (!family) {
                    printf("Family not found!\n");
                    break;
                }
            
                printf("Available categories:\n");
                for (int i = 0; i < CATEGORY_COUNT; i++) {
                    printf("%s\n", category_names[i]);
                }
                printf("Enter category name: ");
                scanf("%s", category);
                
                get_categorical_expense(family, category);
                break;
            }
            
            case 10: {//highest expense day
                int family_id;
                
                printf("Enter family ID: ");
                scanf("%d", &family_id);
                FamilyNode* family = search_family(db, family_id);
                if (!family) {
                    printf("Family not found!\n");
                    break;
                }
                
                get_highest_expense_day(family);
                break;
            }
            case 11: { 
                int date1, date2;
                
                printf("Enter start date (DDMMYYYY): ");
                scanf("%d", &date1);
                printf("Enter end date (DDMMYYYY): ");
                scanf("%d", &date2);
            
                if (!db || !db->root) {
                    printf("No families !\n");
                    
                }
                else{
                BPlusTreeNode* node = db->root;
                while (!node->is_leaf) {
                    node = node->children[0];
                }
                printf("expenses between the dates are :\n");
                while (node != NULL) {
                    for (int i = 0; i < node->num_keys; i++) {
                        FamilyNode* family = node->data_ptrs[i];
                        if (family) {
                            // printf("Family ID: %d\n", 
                            //        family->family_id);
                            get_expense_in_period(family, date1, date2);
                        }
                    }
                    node = node->next; 
                }
            }
                break;
            }
            
            case 12: {//expense in range
                int family_id, individual_id, expense_id1, expense_id2;
                
                printf("Enter family ID: ");
                scanf("%d", &family_id);
                FamilyNode* family = search_family(db, family_id);
                if (!family) {
                    printf("Family not found!\n");
                    break;
                }
            
                printf("Enter member ID: ");
                scanf("%d", &individual_id);
                IndividualNode* member = search_individual(family->members, individual_id);
                if (!member) {
                    printf("Member not found!\n");
                    break;
                }
            
                printf("Enter starting expense ID: ");
                scanf("%d", &expense_id1);
                printf("Enter ending expense ID: ");
                scanf("%d", &expense_id2);
                
                get_expense_in_range(member, expense_id1, expense_id2);
                break;
            }
           
            case 13: // Exit
                running = 0;
                printf("Exiting...\n");
                break;
                
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
    
    save_database(db);
    return 0;
}