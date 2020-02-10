/** program to demonstrate character input output 
 * using getchar() and putchar() function.
 * */

#include <stdio.h>

int main()
{
    char grade;
    int num1=0,num2=0,sum=0;
    printf("Enter two numbers ");
    scanf("%d%d",&num1,&num2);
    sum = num1+num2;
    printf("Sum = %d",sum);
    /* Input character using getchar() and store in grade */
    grade = getchar();
     
    /* Output grade using putchar() */
    putchar(grade);

    return 0;
}
