#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int id;
    char name[32];
} Student;

Student *create_student(int id, const char *name)
{
    Student *student = malloc(sizeof(Student));

    if (student == NULL)
        return NULL;

    student->id = id;
    strncpy(student->name, name, sizeof(student->name) - 1);
    student->name[sizeof(student->name) - 1] = '\0';

    return student;
}

int main(void)
{
    printf("=== Memory Leak Training ===\n");

    /* ---------------------------------------------------------
     * Allocation #1
     * --------------------------------------------------------- */
    Student *student1 = create_student(1, "Alice");

    if (student1 == NULL)
        return EXIT_FAILURE;

    printf("Student 1: %d %s\n",
           student1->id,
           student1->name);


    /* ---------------------------------------------------------
     * student1 is never freed
     * --------------------------------------------------------- */


    /* ---------------------------------------------------------
     * Allocation #2
     * --------------------------------------------------------- */
    Student *student2 = create_student(2, "Bob");

    if (student2 == NULL)
        return EXIT_FAILURE;

    printf("Student 2: %d %s\n",
           student2->id,
           student2->name);


    /* ---------------------------------------------------------
     * BUG #2:
     * We lose the original pointer before freeing it.
     * --------------------------------------------------------- */

    student2 = create_student(3, "Charlie");

    printf("Student 3: %d %s\n",
           student2->id,
           student2->name);


    /* ---------------------------------------------------------
     * BUG #3:
     * Free the memory...
     * --------------------------------------------------------- */

    free(student2);


    /* ---------------------------------------------------------
     * ...but then access it again.
     * => USE-AFTER-FREE
     * --------------------------------------------------------- */

    printf("After free: %s\n", student2->name);


    return EXIT_SUCCESS;
}