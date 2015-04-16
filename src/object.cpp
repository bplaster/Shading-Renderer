#include "object.h"
#include "canvas.h"
#include "core/geometry.h"

rigidhdl::rigidhdl()
{

}

rigidhdl::~rigidhdl()
{

}

/* draw
 *
 * Draw a rigid body.
 */
void rigidhdl::draw(canvashdl *canvas)
{
    // TODO Assignment 1: Send the rigid body geometry to the renderer
    canvas->draw_triangles(geometry, indices);
}

objecthdl::objecthdl()
{
	position = vec3f(0.0, 0.0, -2.0);
	orientation = vec3f(0.0, 0.0, 0.0);
	bound = vec6f(1.0e6, -1.0e6, 1.0e6, -1.0e6, 1.0e6, -1.0e6);
	scale = 1.0;
}

objecthdl::objecthdl(const objecthdl &o)
{
	position = o.position;
	orientation = o.orientation;
	bound = o.bound;
	scale = o.scale;
	rigid = o.rigid;
	for (map<string, materialhdl*>::const_iterator i = o.material.begin(); i != o.material.end(); i++)
		material.insert(pair<string, materialhdl*>(i->first, i->second->clone()));
}

objecthdl::~objecthdl()
{
	for (map<string, materialhdl*>::iterator i = material.begin(); i != material.end(); i++)
		if (i->second != NULL)
		{
			delete i->second;
			i->second = NULL;
		}

	material.clear();
}

/* draw
 *
 * Draw the model. Don't forget to apply the transformations necessary
 * for position, orientation, and scale.
 */
void objecthdl::draw(canvashdl *canvas)
{
	// TODO Assignment 1: Send transformations and geometry to the renderer to draw the object
    canvas->translate(position);
    canvas->scale(vec3f(scale, scale, scale));
    canvas->rotate(orientation[2], vec3f(0.,0.,1.));
    canvas->rotate(orientation[1], vec3f(0.,1.,0.));
    canvas->rotate(orientation[0], vec3f(1.,0.,0.));
    
    for (vector<rigidhdl>::iterator iter = rigid.begin(); iter != rigid.end(); ++iter) {
        // TODO Assignment 2: Pass the material as a uniform into the renderer
        canvas->uniform["material"] = material[(*iter).material];
        
        (*iter).draw(canvas);
        
        // TODO Assignment 2: clear the material in the uniform list
        canvas->uniform.erase("material");
    }
    
    // Undo transformations
    canvas->rotate(-orientation[0], vec3f(1.,0.,0.));
    canvas->rotate(-orientation[1], vec3f(0.,1.,0.));
    canvas->rotate(-orientation[2], vec3f(0.,0.,1.));
    canvas->scale(vec3f(1./scale, 1./scale, 1./scale));
    canvas->translate(-position);

}

/* draw_bound
 *
 * Create a representation for the bounding box and
 * render it.
 */
void objecthdl::draw_bound(canvashdl *canvas)
{
	/* TODO Assignment 1: Generate the geometry for the bounding box and send the necessary
	 * transformations and geometry to the renderer
	 */

    canvas->translate(position);
    canvas->scale(vec3f(scale, scale, scale));
    canvas->rotate(orientation[2], vec3f(0.,0.,1.));
    canvas->rotate(orientation[1], vec3f(0.,1.,0.));
    canvas->rotate(orientation[0], vec3f(1.,0.,0.));
    
    vector<vec8f> bound_geometry;
    vector<int> bound_indices;

    bound_geometry.reserve(8);
    bound_indices.reserve(24);
    
    // Box geometry
    // bound(left, right, bottom, top, front, back)
    bound_geometry.push_back(vec8f(bound[0], bound[3], bound[4], 0.0, 0.0, 0.0, 0.0, 0.0)); // LTF 0
    bound_geometry.push_back(vec8f(bound[0], bound[2], bound[4], 0.0, 0.0, 0.0, 0.0, 0.0)); // LBF 1
    bound_geometry.push_back(vec8f(bound[0], bound[3], bound[5], 0.0, 0.0, 0.0, 0.0, 0.0)); // LTB 2
    bound_geometry.push_back(vec8f(bound[0], bound[2], bound[5], 0.0, 0.0, 0.0, 0.0, 0.0)); // LBB 3
    bound_geometry.push_back(vec8f(bound[1], bound[3], bound[4], 0.0, 0.0, 0.0, 0.0, 0.0)); // RTF 4
    bound_geometry.push_back(vec8f(bound[1], bound[2], bound[4], 0.0, 0.0, 0.0, 0.0, 0.0)); // RBF 5
    bound_geometry.push_back(vec8f(bound[1], bound[3], bound[5], 0.0, 0.0, 0.0, 0.0, 0.0)); // RTB 6
    bound_geometry.push_back(vec8f(bound[1], bound[2], bound[5], 0.0, 0.0, 0.0, 0.0, 0.0)); // RBB 7

    // Indices
    bound_indices.push_back(0);
    bound_indices.push_back(1);
    bound_indices.push_back(0);
    bound_indices.push_back(2);
    bound_indices.push_back(0);
    bound_indices.push_back(4);
    
    bound_indices.push_back(1);
    bound_indices.push_back(3);
    bound_indices.push_back(1);
    bound_indices.push_back(5);
    
    bound_indices.push_back(2);
    bound_indices.push_back(3);
    bound_indices.push_back(2);
    bound_indices.push_back(6);
    
    bound_indices.push_back(3);
    bound_indices.push_back(7);
    
    bound_indices.push_back(4);
    bound_indices.push_back(5);
    bound_indices.push_back(4);
    bound_indices.push_back(6);
    
    bound_indices.push_back(5);
    bound_indices.push_back(7);
    
    bound_indices.push_back(6);
    bound_indices.push_back(7);

    // Give bound material
    uniformhdl *bound_material = new uniformhdl();
    bound_material->emission = vec3f(1.0,1.0,1.0);
    canvas->uniform["material"] = bound_material;

    // Draw bounding lines
    canvas->draw_lines(bound_geometry, bound_indices);
    
    canvas->uniform.erase("material");
    
    // Undo transformations
    canvas->rotate(-orientation[0], vec3f(1.,0.,0.));
    canvas->rotate(-orientation[1], vec3f(0.,1.,0.));
    canvas->rotate(-orientation[2], vec3f(0.,0.,1.));
    canvas->scale(vec3f(1./scale, 1./scale, 1./scale));
    canvas->translate(-position);
}

/* draw_normals
 *
 * create a representation of the normals for this object.
 * If face is false, render the vertex normals. Otherwise,
 * calculate the normals for each face and render those.
 */
void objecthdl::draw_normals(canvashdl *canvas, bool face)
{
    canvas->translate(position);
    canvas->scale(vec3f(scale, scale, scale));
    canvas->rotate(orientation[2], vec3f(0.,0.,1.));
    canvas->rotate(orientation[1], vec3f(0.,1.,0.));
    canvas->rotate(orientation[0], vec3f(1.,0.,0.));
    
	/* TODO Assignment 1: Generate the geometry to display the normals and send the necessary
	 * transformations and geometry to the renderer
	 */

    if (face) {
        for (vector<rigidhdl>::iterator iter = rigid.begin(); iter != rigid.end(); ++iter) {
            vector<vec8f> geometry;
            vector<int> indices;
            int size = (int)(*iter).indices.size();
            geometry.reserve(2*size/3);
            indices.reserve(2*size/3);
            
            for (int i = 0; i < size - 2; i+=3) {
                vec8f avg_point =   (*iter).geometry[(*iter).indices[i]] +
                                    (*iter).geometry[(*iter).indices[i+1]] +
                                    (*iter).geometry[(*iter).indices[i+2]];
                avg_point /= 3.;
                
                vec8f norm_point = avg_point;
                vec3f norm_vec = avg_point(3,6);
                norm_vec /= 4.;
                norm_point.set(0, 3, avg_point(0,3) + norm_vec);
                geometry.push_back(avg_point);
                geometry.push_back(norm_point);
                indices.push_back(2*i/3);
                indices.push_back(2*i/3 + 1);
            }
            
            canvas->draw_lines(geometry, indices);
        }
    } else {
        for (vector<rigidhdl>::iterator iter = rigid.begin(); iter != rigid.end(); ++iter) {
            vector<vec8f> geometry;
            vector<int> indices;
            int size = (int)(*iter).geometry.size();
            geometry.reserve(2*size);
            indices.reserve(2*size);

            for (int i = 0; i < size; i++) {
                vec8f norm_point = (*iter).geometry[i];
                vec3f normal = (*iter).geometry[i](3,6);
                normal /= 4.;
                normal = normal + norm_point(0,3);
                norm_point.set(0, 3, normal);
                geometry.push_back((*iter).geometry[i]);
                geometry.push_back(norm_point);
                indices.push_back(2*i);
                indices.push_back(2*i+1);
            }
            
            canvas->draw_lines(geometry, indices);
        }
    }

    
    // Undo transformations
    canvas->rotate(-orientation[0], vec3f(1.,0.,0.));
    canvas->rotate(-orientation[1], vec3f(0.,1.,0.));
    canvas->rotate(-orientation[2], vec3f(0.,0.,1.));
    canvas->scale(vec3f(1./scale, 1./scale, 1./scale));
    canvas->translate(-position);
}


