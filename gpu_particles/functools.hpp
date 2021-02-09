#pragma once
#include <fstream>

string get_content_from_file(const string filepath)
{
    ifstream fs(filepath, ios::in | ios::binary);

    if (!fs.is_open())
    {
        cout << ">> Cannot open file " + filepath << endl;
        return "";
    }

    string content;

    fs.seekg(0, fs.end);
    content.reserve(fs.tellg());
    fs.seekg(0, fs.beg);

    content.assign((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

    fs.close();

    return content;
}