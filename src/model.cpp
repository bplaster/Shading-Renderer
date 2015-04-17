/*
 * model.cpp
 *
 *  Created on: Dec 3, 2014
 *      Author: nbingham
 */

#include "model.h"
#include "standard.h"

modelhdl::modelhdl()
{
    
}

modelhdl::modelhdl(string filename)
{
    load_obj(filename);
}

modelhdl::~modelhdl()
{
    
}

/* load_obj
 *
 * Load the .obj file located at 'filename'. You only
 * need to handle the following commands:
 * mtllib - the location of the material template library
 * usemtl - use the material named X
 * g - name this new rigid body X
 * v - add a vertex to the current rigid body
 * vn - add a normal to the current rigid body
 * vt - add a texture coordinate to the current rigid body
 * f - add a face to the current rigid body
 *
 * The underlying storage of data in an object file is slightly
 * different from the one we are using here. You may change your
 * representation if you like, but just remember to account for this
 * in some way.
 */
void modelhdl::load_obj(string filename)
{
    int s = 4;
    float x, y, z;
    int v, n, t;
    char name[32];
    
    ifstream fin(filename.c_str());
    if (!fin.is_open())
    {
        cerr << "Error: file not found: " << filename << endl;
        return;
    }
    
    vector<vec3f> vertices;
    vector<vec3f> normals;
    vector<vec2f> texcoords;
    vec3f ave(0.0, 0.0, 0.0);
    float num = 0;
    
    string line(256, '\0');
    string command;
    while (getline(fin, line))
    {
        istringstream iss(line);
        
        if (iss >> command)
        {
            if (command == "mtllib")
            {
                string mtlname;
                iss >> mtlname;
                
                if (mtlname.size() > 0 && mtlname[0] != '/')
                {
                    int idx = filename.find_last_of("/");
                    if (idx == string::npos)
                        idx = filename.find_last_of("\\");
                    mtlname = filename.substr(0, idx) + "/" + mtlname;
                }
                
                load_mtl(mtlname);
            }
            else if (command == "g")
            {
                rigid.push_back(rigidhdl());
                if (rigid.size() > 1)
                    rigid[rigid.size()-1].material = rigid[rigid.size()-2].material;
            }
            else if (command == "usemtl")
            {
                if (rigid.size() == 0)
                    rigid.push_back(rigidhdl());
                
                iss >> rigid.back().material;
            }
            else if (command == "v" || command == "vn" || command == "vt" || command == "f")
            {
                if (rigid.size() == 0)
                    rigid.push_back(rigidhdl());
                
                if (command == "v" && iss >> x >> y >> z)
                {
                    vertices.push_back(vec3f(x, y, z));
                    ave += vec3f(x, y, z);
                    num += 1.0;
                }
                else if (command == "vn" && iss >> x >> y >> z)
                    normals.push_back(vec3f(x, y, z));
                else if (command == "vt" && iss >> x >> y)
                    texcoords.push_back(vec2f(x, y));
                else if (command == "f")
                {
                    int first = rigid.back().geometry.size();
                    string part;
                    int i = 0;
                    while (iss >> part)
                    {
                        vec8f point(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
                        v = -1;
                        n = -1;
                        t = -1;
                        
                        if (sscanf(part.c_str(), "%d/%d/%d", &v, &t, &n) == 3)
                        {
                            point.set(0,3, vertices[v-1]);
                            point.set(6,8, texcoords[t-1]);
                            point.set(3,6, normals[n-1]);
                        }
                        else if (sscanf(part.c_str(), "%d//%d", &v, &n) == 2)
                        {
                            point.set(0,3, vertices[v-1]);
                            point.set(3,6, normals[n-1]);
                        }
                        else if (sscanf(part.c_str(), "%d/%d", &v, &t) == 2)
                        {
                            point.set(0,3, vertices[v-1]);
                            point.set(6,8, texcoords[t-1]);
                        }
                        else if (sscanf(part.c_str(), "%d", &v) == 1)
                            point.set(0,3, vertices[v-1]);
                        
                        bound[0] = min(bound[0], point[0]);
                        bound[1] = max(bound[1], point[0]);
                        bound[2] = min(bound[2], point[1]);
                        bound[3] = max(bound[3], point[1]);
                        bound[4] = min(bound[4], point[2]);
                        bound[5] = max(bound[5], point[2]);
                        
                        rigid.back().geometry.push_back(point);
                        
                        if (i >= 2)
                        {
                            rigid.back().indices.push_back(first);
                            rigid.back().indices.push_back(rigid.back().geometry.size()-1);
                            rigid.back().indices.push_back(rigid.back().geometry.size()-2);
                        }
                        i++;
                    }
                }
            }
        }
    }
    
    ave /= num;
    
    for (int i = 0; i < 6; i++)
        bound[i] -= ave[i/2];
    
    for (int k = 0; k < rigid.size(); k++)
        for (int i = 0; i < rigid[k].geometry.size(); i++)
            for (int j = 0; j < 3; j++)
                rigid[k].geometry[i][j] -= ave[j];
    
    fin.close();
}

/* load_mtl
 *
 * Load the .mtl file located at 'filename'.
 */
void modelhdl::load_mtl(string filename)
{
    ifstream fin(filename.c_str());
    if (!fin.is_open())
    {
        cerr << "Error: file not found: " << filename << endl;
        return;
    }
    
    string current_material = "";
    string type = "";
    string line(256, '\0');
    string command;
    while (getline(fin, line))
    {
        istringstream iss(line);
        
        if (iss >> command)
        {
            if (command == "newmtl")
            {
                iss >> type;
                iss >> current_material;
                if (type == "uniform")
                    material[current_material] = new uniformhdl();
                else if (type == "non_uniform")
                    material[current_material] = new nonuniformhdl();
            }
            else if (command == "Ke" && type == "uniform")
                iss >> ((uniformhdl*)material[current_material])->emission[0] >> ((uniformhdl*)material[current_material])->emission[1] >> ((uniformhdl*)material[current_material])->emission[2];
            else if (command == "Ka" && type == "uniform")
                iss >> ((uniformhdl*)material[current_material])->ambient[0] >> ((uniformhdl*)material[current_material])->ambient[1] >> ((uniformhdl*)material[current_material])->ambient[2];
            else if (command == "Kd" && type == "uniform")
                iss >> ((uniformhdl*)material[current_material])->diffuse[0] >> ((uniformhdl*)material[current_material])->diffuse[1] >> ((uniformhdl*)material[current_material])->diffuse[2];
            else if (command == "Ks" && type == "uniform")
                iss >> ((uniformhdl*)material[current_material])->specular[0] >> ((uniformhdl*)material[current_material])->specular[1] >> ((uniformhdl*)material[current_material])->specular[2];
            else if (command == "Ns" && type == "uniform")
                iss >> ((uniformhdl*)material[current_material])->shininess;
        }
    }
}
