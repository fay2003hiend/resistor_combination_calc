#include <stdlib.h>

#include <QApplication>
#include <array>
#include <iostream>
#include <vector>

#include "form_rescalc.h"

using namespace std;

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    FormResCalc f;
    f.show();

    return a.exec();
}
