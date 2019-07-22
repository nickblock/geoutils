#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main(int argi, char** argc) {

  if(argi < 2) {
    cout << "not enough args" << endl;
    return 1;
  }
  try {
    int size = std::stoi(argc[1]);
    vector<uint32_t> heatmap(size*size);

    for(auto& cell : heatmap) {
      cell = 0;
    }

    cout << "Done" << endl;
  }
  catch (exception& ex) {
    cout << "Excp " << ex.what() << endl;

    return 1;
  }

  return 0;
}