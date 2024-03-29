#ifndef COMMONVARS_H
#define COMMONVARS_H

#include <string>
#include <cstdio>
#include <map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <set>
using namespace std;

const float EPS = 1e-9;

#pragma region windows related variables
const unsigned int SCR_WIDTH = 900;
const unsigned int SCR_HEIGHT = SCR_WIDTH;
const unsigned int TOTAL_PIXELS = SCR_WIDTH * SCR_HEIGHT;
#pragma endregion

#pragma region rendering related
const unsigned int MAX_FRAGMENT_NUM = (unsigned int)1e7;
const unsigned int RESTART_NUM = 0x5FFFFFu;//primitive restart number
#pragma endregion

#endif // !COMMONVARS_H
