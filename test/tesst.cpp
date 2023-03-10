#include <iostream>
class A
{
private:
    //为什么?
    //访问限制标号是针对类而不是针对一个类的不同对象，只要同属一个类就可以不用区分同一个类的不同对象。因为CExample(const CExample& RightSides) 是类的成员函数，所以有权限访问私有数据成员。如果是在main函数中直接RightSides.nSize，那肯定就会报错了，不能访问，因为这是在类外不能访问私有数据成员。一个类的成员函数可以访问这个类的私有数据成员，我们要理解这个对类的访问限制，而不是针对对象。
 

    static void test1()
    {
        A *a = new A();
        a->m_a = 2;
    }
    void test2()
    {
        A *a = new A();
        a->m_a = 2;
    }
private:
    int m_a;
};

int main()
{
    return 0;
}