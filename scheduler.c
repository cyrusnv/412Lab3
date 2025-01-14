#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"

// Type definitions
typedef struct Graph Graph; // defines the dependence graph
typedef struct Node Node;   // defines a node in the dg

// Renaming functions
void rename_registers(struct Instruction *ir);
int handleThree(struct Instruction *currOp, int VRName, int index);
int handleStore(struct Instruction *currOp, int VRName, int index);
int handleLoad(struct Instruction *currOp, int VRName, int index);
int handleLoadI(struct Instruction *currOp, int VRName);

// Functions for building the dependence graph
Graph *build_dp(struct Instruction *ir);
void set_mv(int n);
int get_mv(int n);
Graph *createGraph(int ncount);
Node *createNode(int v);
int addNode(Graph *graph, int node, int opcode);
void writeToDotFile(Graph *g, struct Instruction *ir);

// Debugging functions
void print_renamed_code_vr(struct Instruction *ir);
void print_renamed_code_pr(struct Instruction *ir);

// Global Maps
int *SRtoVR = NULL;
int *LU = NULL;

/* Global Variables */
// These are global variables that survived from Lab 2.
// Many are probably not useful.
int maxlive = 0;
int livecount = 0;
int prcount;
int usableprcount;
struct Instruction *startir;

/* Global variables that are actually new to Lab 3 */
static int MAX_VERTICES = 0; // max vertices in dependence graph

// graph functions
struct Graph *createGraph(int vertices);
int addEdge(Graph *graph, int src, int dest);
void getSuccessors(Graph *graph, int vertex);
void getPredecessors(Graph *graph, int vertex);
void freeGraph(Graph *graph);

// A node in the adjacency list.
struct Node
{
    int vertex;
    int opcode;
    struct Node *next;
};

// This runs the program. Duh.
int main(int argc, char **argv)
{
    // Handle -h flag
    if (argc > 1 && strcmp(argv[1], "-h") == 0)
    {
        printf("YOU SHOULD PROBABLY HANDLE AN H FLAG\n");
        return 0;
    }
    else if (argc > 1 && strcmp(argv[1], "-x") == 0)
    {
        printf("You might even have to handle a -x flag.\n");
    }
    else if (argc > 1)
    {

        // Parse k value
        // printf("Code Check 2:\n");
        prcount = atoi(argv[1]);
        if (prcount < 3 || prcount > 64)
        {
            fprintf(stderr, "ERROR: k must be between 3 and 64\n");
            return 1;
        }
    }

    // TODO: You should probably check to be sure the file is valid.
    struct Instruction *ir = buildIR(argv[2]);
    startir = ir;
    if (ir == NULL)
    {
        return 1; // Error already printed by buildIR
    }

    // Perform register renaming
    rename_registers(ir);

    // We can now set the size of the graph
    set_mv(opcount + 10);
    printf("MAX_VERTICES: %d\n", MAX_VERTICES);
    // Take a wild guess what this function does.
    Graph *dp = build_dp(ir);

    writeToDotFile(dp, ir);
    return 0;
}

/*
 * Requires:
 *      ir -- An instruction struct representing the first instruction
 *              in the input program IR.
 *
 * Effects:
 *      Builds a dependence graph for the input IR. Returns 0 on success;
 *      returns a non-zero value on failure.
 */
Graph *build_dp(struct Instruction *ir)
{
    Graph *dgraph = createGraph(MAX_VERTICES);

    // Set up DP map
    int VRtoOpsize = (opcount * 3) + 10;
    int *VRtoOp = malloc(VRtoOpsize * sizeof(int));
    for (int i = 0; i < VRtoOpsize; i++)
    {
        VRtoOp[i] = -1;
    }

    // let's pseudocode our way through this, for now.
    // mr_store = ...
    // load_list = ...
    // output_list = ...

    struct Instruction *currOp = ir->next;
    while (currOp->line != 0)
    {
        // Create a node for op
        int op = currOp->line;
        // Node *node = createNode(op); // (We'll see if we need this)
        addNode(dgraph, op, currOp->opcode);

        // Go over each definition (stores do not have a definition)
        if (currOp->opcode != STORE)
        {
            VRtoOp[currOp->vr3] = op;
        }

        // Go over each use (LOADI has no uses)
        if (currOp->opcode != LOADI)
        {
            int vr = currOp->vr1;
            addEdge(dgraph, op, VRtoOp[vr]);

            // Go over the second use (neither LOAD nor LOADI)
            if (currOp->opcode != LOAD)
            {
                int vr2 = currOp->vr2;
                addEdge(dgraph, op, VRtoOp[vr2]);
            }
        }

        /* Add serial and conflict edges */
        int mr_store = -1; // Store the line of the most recent ILOC STORE
        // Repurpose the Node structure to keep track of recent LOADs and OUTPUTs
        Node *load_list_head = createNode(-7);
        Node *output_list_head = createNode(-8);
        switch (currOp->opcode)
        {
        case LOAD:
            if (mr_store != -1)
            {
                addEdge(dgraph, op, mr_store);
            }
            break;
        case OUTPUT:
            if (mr_store != -1)
            {
                addEdge(dgraph, op, mr_store);
            }
            if (output_list_head->next != NULL)
            {
                addEdge(dgraph, op, output_list_head->next->vertex);
            }
            break;
        case STORE:
            if (mr_store != -1)
            {
                addEdge(dgraph, op, mr_store);
            }
            // add serialization edges based on previous stores and outputs.
            Node *edgeNode = load_list_head->next;
            while (edgeNode != NULL)
            {
                addEdge(dgraph, op, edgeNode->vertex);
                edgeNode = edgeNode->next;
            }
            edgeNode = output_list_head;
            while (edgeNode != NULL)
            {
                addEdge(dgraph, op, edgeNode->vertex);
                edgeNode = edgeNode->next;
            }
        }

        if (currOp->opcode == STORE)
            mr_store = op;

        currOp = currOp->next;
    }

    // printf("opcount: %d\n", opcount);

    return (dgraph);
}

/* Graph Management Functions and Definitions */

/* Graph structure, based on an adjacency list. Both forwards and backwards
 * graphs for predecessors and successors.
 */
struct Graph
{
    int nodeCount;

    Node **forwardgraph;
    Node **backwardgraph;
};

/*
 * Requires:
 *  - v: an int representing the operation of the node being created.
 *
 * Effects:
 *  - Returns a node with a vertex of v value with no successor. Opcode is NULL.
 */
Node *createNode(int v)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->vertex = v;
    newNode->next = NULL;
    newNode->opcode = -28;
    return newNode;
}

/*
 * Requires:
 *  - v: an int representing the number of vertices (nodes) in the graph.
 *
 * Effects:
 *  - Creates a graph of type Graph with the given number of nodes as an adjacency
 *      list; nodes are initially given value NULL.
 */
Graph *createGraph(int ncount)
{
    if (MAX_VERTICES == 0)
    {
        printf("createGraph: MAX_VERTICES really should not be 0. It is.\n");
        return (NULL);
    }
    else if (ncount > MAX_VERTICES)
    {
        printf("createGraph: ncount > MAX_VERTICES. That's bad.\n");
        return (NULL);
    }

    Graph *dgraph = (Graph *)malloc(sizeof(Graph));
    dgraph->nodeCount = ncount;

    // Allocate arrays
    dgraph->forwardgraph = (Node **)malloc(ncount * sizeof(Node *));
    dgraph->backwardgraph = (Node **)malloc(ncount * sizeof(Node *));

    // Initialize with dummy heads
    for (int i = 0; i < ncount; i++)
    {
        dgraph->forwardgraph[i] = createNode(-1);  // -1 indicates dummy head that is NOT IN GRAPH
        dgraph->backwardgraph[i] = createNode(-1); // -1 indicates dummy head that is NOT IN GRAPH
    }

    return (dgraph);
}

/*
 *  Requires:
 *      - graph: the Graph to which the node is being added
 *      - node: an int representing the value of the node being added.
 *
 *  Effects:
 *      - Updates the dummy head at index node of the forwards and backwards
 *          graphs to have value -2, which indicates that the node is present
 *          in the graph.
 */
int addNode(Graph *graph, int node, int opcode)
{

    // TODO: add error checking. You should learn to do this in the moment, but...
    if (node < 0 || node >= graph->nodeCount) {
        fprintf(stderr, "Error: vertex index out of bounds\n");
        return (1);
    }

    graph->forwardgraph[node]->vertex = -2;
    graph->forwardgraph[node]->opcode = opcode;
    graph->backwardgraph[node]->vertex = -2;
    graph->backwardgraph[node]->opcode = opcode;

    return (0);
}

int addEdge(Graph *graph, int src, int dest)
{

    // Ensure that the nodes exists.
    if (graph->forwardgraph[src]->vertex == -1 || graph->backwardgraph[dest]->vertex == -1)
    {
        printf("addEdge: Trying to add an edge that doesn't exist (%d, %d).\n", src, dest);
        return (1);
    }

    // Create the new nodes
    Node *fnode = createNode(src);
    Node *bnode = createNode(dest);

    // Update the forward graph.
    fnode->next = graph->forwardgraph[src]->next;
    graph->forwardgraph[src]->next = fnode;

    // Update the backwards graph.
    bnode->next = graph->backwardgraph[dest]->next;
    graph->backwardgraph[dest]->next = bnode;

    return (0);
}

void set_mv(int n) {
    MAX_VERTICES = n;
}

// Prints a graph to a dot file. For visualization purposes.
/* Draft one this sucks
void printGraph(Graph *g)
{
    FILE *dotfile;

    dotfile = fopen("dpgraph.dot", "w"); // Open for writing

    if (dotfile == NULL)
    {
        printf("Error opening file!\n");
        return 1; // Indicate an error
    }

    fprintf(dotfile, "digraph DG {\n");
    Node *currNode = g->forwardgraph[0];
    for (int i = 0; i < MAX_VERTICES; i++)
    {
        if (g->forwardgraph[i]->vertex == -2)
        {
            fprintf(dotfile, " %d [label=\"%d: ]");
        }
    }

    fprintf(dotfile, "}");
    fclose(dotfile);
}
*/

void printGraph(Graph *g, struct Instruction *ir, FILE *dotfile) {
    if (dotfile == NULL) {
        printf("Error: No file provided for graph output\n");
        return;
    }

    // Start the digraph
    fprintf(dotfile, "digraph DG {\n");

    // First pass: Print all nodes with their ILOC instructions
    struct Instruction *curr = ir->next;
    while (curr->line != 0) {
        if (g->forwardgraph[curr->line]->vertex == -2) { // Node exists in graph
            fprintf(dotfile, "    %d [label=\"%d: ", curr->line, curr->line);
            
            // Format the ILOC instruction based on opcode
            switch(curr->opcode) {
                case LOADIL:
                    fprintf(dotfile, "loadI %d => r%d", curr->sr1, curr->sr3);
                    break;
                case LOAD:
                    fprintf(dotfile, "load r%d => r%d", curr->sr1, curr->sr3);
                    break;
                case STORE:
                    fprintf(dotfile, "store r%d => r%d", curr->sr1, curr->sr3);
                    break;
                case ADD:
                    fprintf(dotfile, "add r%d, r%d => r%d", curr->sr1, curr->sr2, curr->sr3);
                    break;
                case SUB:
                    fprintf(dotfile, "sub r%d, r%d => r%d", curr->sr1, curr->sr2, curr->sr3);
                    break;
                case MULT:
                    fprintf(dotfile, "mult r%d, r%d => r%d", curr->sr1, curr->sr2, curr->sr3);
                    break;
                case LSHIFT:
                    fprintf(dotfile, "lshift r%d, r%d => r%d", curr->sr1, curr->sr2, curr->sr3);
                    break;
                case RSHIFT:
                    fprintf(dotfile, "rshift r%d, r%d => r%d", curr->sr1, curr->sr2, curr->sr3);
                    break;
                case OUTPUT:
                    fprintf(dotfile, "output %d", curr->sr1);
                    break;
                case NOPL:
                    fprintf(dotfile, "nop");
                    break;
            }
            fprintf(dotfile, "\"];\n");
        }
        curr = curr->next;
    }

    // Second pass: Print all edges with dependency information
    // Data dependencies are typically from use to definition
    for (int i = 1; i < g->nodeCount; i++) {
        if (g->forwardgraph[i]->vertex == -2) {
            Node *edge = g->forwardgraph[i]->next;
            while (edge != NULL) {
                int src = i;
                int dest = edge->vertex;
                
                // Determine edge type
                if (edge->opcode == -1) { // Serialization edge
                    fprintf(dotfile, "    %d -> %d [label=\" Serial\"];\n", src, dest);
                } else {
                    // For data dependencies, show the register that creates the dependency
                    struct Instruction *srcInst = ir;
                    while (srcInst != NULL && srcInst->line != src) {
                        srcInst = srcInst->next;
                    }
                    if (srcInst != NULL) {
                        fprintf(dotfile, "    %d -> %d [label=\" Data, vr%d\"];\n", 
                                src, dest, srcInst->vr3);
                    }
                }
                edge = edge->next;
            }
        }
    }

    // Third pass: Add any special edges (like Conflict edges for memory operations)
    curr = ir->next;
    while (curr->line != 0) {
        if (curr->opcode == OUTPUT) {
            // Add conflict edges from output to any previous stores
            struct Instruction *prev = ir->next;
            while (prev->line != curr->line) {
                if (prev->opcode == STORE) {
                    fprintf(dotfile, "    %d -> %d [label=\" Conflict\"];\n", 
                            curr->line, prev->line);
                }
                prev = prev->next;
            }
        }
        curr = curr->next;
    }

    fprintf(dotfile, "}\n");
    fflush(dotfile);
}

// Helper function to write graph to a DOT file
void writeToDotFile(Graph *g, struct Instruction *ir) {
    FILE *dotfile = fopen("dgraph.dot", "w");
    if (dotfile == NULL) {
        printf("Error opening file %s for writing\n", "dgraph.dot");
        return;
    }
    
    printGraph(g, ir, dotfile);
    fclose(dotfile);
    
    printf("Graph written to %s\n", "dgraph.dot");
    printf("To visualize, use command: dot -Tpdf %s -o graph.pdf\n", "dgraph.dot");
}

/* BELOW: RENAMING CODE THAT I HOPEFULLY DO NOT HAVE TO TOUCH. */

// Function to perform register renaming
void rename_registers(struct Instruction *ir)
{

    // Your counter for VRnames
    int VRName = 0;
    // Set up your renaming maps...
    // int mapsize = opcount * 3;
    int mapsize = (maxregister + 1) * 3;
    SRtoVR = malloc(mapsize * sizeof(int));
    LU = malloc(mapsize * sizeof(int));
    for (int i = 0; i < mapsize; i++)
    {
        SRtoVR[i] = -1;
        LU[i] = -1;
    }

    // Main loop over the operations, bottom to top
    struct Instruction *currOp = ir->prev->prev;
    int index = (currOp->line) - 1;
    // int iterator = 0;
    while (currOp->line != 0)
    {
        // printf("iterator: %d\n", iterator);
        // iterator++;
        int currCode = currOp->opcode;
        // printf("line: %d\n", currOp->line);
        // printf("currCode: %d\n\n", currCode);

        // There are basically five versions that I'm going to do, based on the operand.
        if (currCode == ADD || currCode == SUB || currCode == MULT || currCode == LSHIFT || currCode == RSHIFT)
        {
            VRName = handleThree(currOp, VRName, index);
        }
        else if (currCode == STORE)
        {
            VRName = handleStore(currOp, VRName, index);
        }
        else if (currCode == LOAD)
        {
            VRName = handleLoad(currOp, VRName, index);
        }
        else if (currCode == LOADIL)
        {
            VRName = handleLoadI(currOp, VRName);
        }

        currOp = currOp->prev;
        index--;

        if (livecount > maxlive)
        {
            maxlive = livecount;
        }
    }
}

/* HELPER FUNCTIONS FOR RENAMING */

int handleThree(struct Instruction *currOp, int VRName, int index)
{
    /* Defined operand */
    if (SRtoVR[currOp->sr3] == -1)
    {
        SRtoVR[currOp->sr3] = VRName;
        VRName++;
        livecount++;
    }
    currOp->vr3 = SRtoVR[currOp->sr3];
    currOp->nu3 = LU[currOp->sr3];
    SRtoVR[currOp->sr3] = -1;
    LU[currOp->sr3] = -1;
    // livecount--;

    /* used operand 1 */
    if (SRtoVR[currOp->sr1] == -1)
    {
        SRtoVR[currOp->sr1] = VRName;
        VRName++;
    }
    currOp->vr1 = SRtoVR[currOp->sr1];
    currOp->nu1 = LU[currOp->sr1];
    // livecount++;

    /* used operand 2 */
    if (SRtoVR[currOp->sr2] == -1)
    {
        SRtoVR[currOp->sr2] = VRName;
        VRName++;
    }
    currOp->vr2 = SRtoVR[currOp->sr2];
    currOp->nu2 = LU[currOp->sr2];
    if (currOp->sr2 != currOp->sr1)
    {
        livecount++;
    }

    /* reset last uses for operands 1 and 2 to be index */
    LU[currOp->sr1] = index;
    LU[currOp->sr2] = index;

    return VRName;
}

int handleStore(struct Instruction *currOp, int VRName, int index)
{
    // NOTE: Treat both operands as uses here!
    /* used operand 1 */
    if (SRtoVR[currOp->sr1] == -1)
    {
        SRtoVR[currOp->sr1] = VRName;
        VRName++;
    }
    currOp->vr1 = SRtoVR[currOp->sr1];
    currOp->nu1 = LU[currOp->sr1];
    livecount++;

    /* used operand 2 */
    if (SRtoVR[currOp->sr3] == -1)
    {
        SRtoVR[currOp->sr3] = VRName;
        VRName++;
    }
    currOp->vr3 = SRtoVR[currOp->sr3];
    currOp->nu3 = LU[currOp->sr3];

    if (currOp->sr1 != currOp->sr3)
    {
        livecount++;
    }

    /* reset last uses for operands 1 and 2 to be index */
    LU[currOp->sr1] = index;
    LU[currOp->sr3] = index;

    return VRName;
}

int handleLoad(struct Instruction *currOp, int VRName, int index)
{
    /* Defined operand */
    if (SRtoVR[currOp->sr3] == -1)
    {
        SRtoVR[currOp->sr3] = VRName;
        VRName++;
        livecount++;
    }
    currOp->vr3 = SRtoVR[currOp->sr3];
    currOp->nu3 = LU[currOp->sr3];
    SRtoVR[currOp->sr3] = -1;
    LU[currOp->sr3] = -1;
    // livecount--;

    /* used operand 1 */
    if (SRtoVR[currOp->sr1] == -1)
    {
        SRtoVR[currOp->sr1] = VRName;
        VRName++;
    }
    currOp->vr1 = SRtoVR[currOp->sr1];
    currOp->nu1 = LU[currOp->sr1];
    // livecount++;

    /* reset last uses for operand 1 to be index */
    LU[currOp->sr1] = index;

    return VRName;
}

int handleLoadI(struct Instruction *currOp, int VRName)
{
    /* Defined operand */
    if (SRtoVR[currOp->sr3] == -1)
    {
        SRtoVR[currOp->sr3] = VRName;
        VRName++;
        livecount++;
    }
    currOp->vr3 = SRtoVR[currOp->sr3];
    currOp->nu3 = LU[currOp->sr3];
    SRtoVR[currOp->sr3] = -1;
    LU[currOp->sr3] = -1;

    livecount--;

    return VRName;
}

/* Print functions */

void print_renamed_code_vr(struct Instruction *ir)
{
    struct Instruction *curr = ir->next; // Skip dummy head
    while (curr->line != -1)
    { // Until we hit end sentinel
        switch (curr->opcode)
        {
        case LOAD:
            printf("load r%d => r%d\n",
                   curr->vr1, curr->vr3);
            break;

        case STORE:
            printf("store r%d => r%d\n",
                   curr->vr1, curr->vr3);
            break;

        case LOADIL:
            printf("loadI %d => r%d\n",
                   curr->sr1, curr->vr3);
            break;

        case ADD:
            printf("add r%d, r%d => r%d\n",
                   curr->vr1, curr->vr2, curr->vr3);
            break;

        case SUB:
            printf("sub r%d, r%d => r%d\n",
                   curr->vr1, curr->vr2, curr->vr3);
            break;

        case MULT:
            printf("mult r%d, r%d => r%d\n",
                   curr->vr1, curr->vr2, curr->vr3);
            break;

        case LSHIFT:
            printf("lshift r%d, r%d => r%d\n",
                   curr->vr1, curr->vr2, curr->vr3);
            break;

        case RSHIFT:
            printf("rshift r%d, r%d => r%d\n",
                   curr->vr1, curr->vr2, curr->vr3);
            break;

        case OUTPUTL:
            printf("output %d\n", curr->sr1);
            break;

        case NOPL:
            printf("nop\n");
            break;
        }
        curr = curr->next;
    }
}

void print_renamed_code_pr(struct Instruction *ir)
{
    struct Instruction *curr = ir->next; // Skip dummy head
    while (curr->line != -1)
    { // Until we hit end sentinel
        switch (curr->opcode)
        {
        case LOAD:
            printf("load r%d => r%d\n",
                   curr->pr1, curr->pr3);
            break;

        case STORE:
            printf("store r%d => r%d\n",
                   curr->pr1, curr->pr3);
            break;

        case LOADIL:
            if (curr->sr1 == 0 && curr->vr1 > 30000)
            {
                printf("loadI %d => r%d\n",
                       curr->vr1, curr->pr3);
            }
            else
            {
                printf("loadI %d => r%d\n",
                       curr->sr1, curr->pr3);
            }

            break;

        case ADD:
            printf("add r%d, r%d => r%d\n",
                   curr->pr1, curr->pr2, curr->pr3);
            break;

        case SUB:
            printf("sub r%d, r%d => r%d\n",
                   curr->pr1, curr->pr2, curr->pr3);
            break;

        case MULT:
            printf("mult r%d, r%d => r%d\n",
                   curr->pr1, curr->pr2, curr->pr3);
            break;

        case LSHIFT:
            printf("lshift r%d, r%d => r%d\n",
                   curr->pr1, curr->pr2, curr->pr3);
            break;

        case RSHIFT:
            printf("rshift r%d, r%d => r%d\n",
                   curr->pr1, curr->pr2, curr->pr3);
            break;

        case OUTPUTL:
            printf("output %d\n", curr->sr1);
            break;

        case NOPL:
            printf("nop\n");
            break;
        }
        curr = curr->next;
    }
}