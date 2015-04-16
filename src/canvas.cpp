#include "canvas.h"
#include "core/geometry.h"
#include "light.h"
#include "material.h"

canvashdl::canvashdl(int w, int h)
{
	last_reshape_time = -1.0;
	width = w;
	height = h;
	reshape_width = w;
	reshape_height = h;

	matrices[viewport_matrix] = mat4f((float)width/2.0, 0.0, 0.0, (float)width/2.0,
									  0.0, (float)height/2.0, 0.0, (float)height/2.0,
									  0.0, 0.0, (float)depth/2.0, (float)depth/2.0,
									  0.0, 0.0, 0.0, 1.0);

	initialized = false;
    planes.reserve(6);

	color_buffer = new unsigned char[width*height*3];
	depth_buffer = new unsigned short[width*height];
    
    // Initialize depth buffer values
    for (int i = 0; i < width*height; i++){
        depth_buffer[i] = depth;
    }

	screen_texture = 0;
	screen_geometry = 0;
	screen_shader = 0;

	active_matrix = modelview_matrix;

	for (int i = 0; i < 4; i++)
		matrices[i] = identity<float, 4, 4>();

	polygon_mode = line;
	shade_model = none;
	culling_mode = backface;
}

canvashdl::~canvashdl()
{
	if (color_buffer != NULL)
	{
		delete [] color_buffer;
		color_buffer = NULL;
	}

	if (depth_buffer != NULL)
	{
		delete [] depth_buffer;
		depth_buffer = NULL;
	}
}

void canvashdl::clear_color_buffer()
{
	memset(color_buffer, 0, width*height*3*sizeof(unsigned char));
}

void canvashdl::clear_depth_buffer()
{
	memset(depth_buffer, 255, width*height*sizeof(unsigned short));
}

void canvashdl::reallocate(int w, int h)
{
	last_reshape_time = -1.0;

	if (color_buffer != NULL)
	{
		delete [] color_buffer;
		color_buffer = NULL;
	}

	if (depth_buffer != NULL)
	{
		delete [] depth_buffer;
		depth_buffer = NULL;
	}

	width = w;
	height = h;

	color_buffer = new unsigned char[w*h*3];
	depth_buffer = new unsigned short[w*h];
    
    // Initialize depth buffer values
    for (int i = 0; i < w*h; i++){
        depth_buffer[i] = depth;
    }

	glActiveTexture(GL_TEXTURE0);
	check_error(__FILE__, __LINE__);
	glBindTexture(GL_TEXTURE_2D, screen_texture);
	check_error(__FILE__, __LINE__);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, color_buffer);
	check_error(__FILE__, __LINE__);
}

/* set_matrix
 *
 * Change which matrix is active.
 */
void canvashdl::set_matrix(matrix_id matid)
{
	// TODO Assignment 1: Change which matrix is active.
    active_matrix = matid;
}

/* load_identity
 *
 * Set the active matrix to the identity matrix.
 * This implements: https://www.opengl.org/sdk/docs/man2/xhtml/glLoadIdentity.xml
 */
void canvashdl::load_identity()
{
	// TODO Assignment 1: Set the active matrix to the identity matrix.
    matrices[active_matrix] = identity<float, 4, 4>();
}

/* rotate
 *
 * Multiply the active matrix by a rotation matrix.
 * This implements: https://www.opengl.org/sdk/docs/man2/xhtml/glRotate.xml
 */
void canvashdl::rotate(float angle, vec3f axis)
{
	// TODO Assignment 1: Multiply the active matrix by a rotation matrix.
    if (angle != 0.0) {
        float c = cosf(angle);
        float cp = 1. - c;
        float s = sinf(angle);
        axis = norm(axis);
        mat4f projection(c + pow(axis[0],2)*cp, axis[0]*axis[1]*cp - axis[2]*s, axis[0]*axis[2]*cp + axis[1]*s, 0.,
                         axis[0]*axis[1]*cp + axis[2]*s, c + pow(axis[1],2)*cp, axis[1]*axis[2]*cp - axis[0]*s, 0.,
                         axis[0]*axis[2]*cp - axis[1]*s, axis[1]*axis[2]*cp + axis[0]*s, c + pow(axis[2],2)*cp, 0.,
                         0., 0., 0., 1.);
        matrices[active_matrix] *= projection;
    }
}

/* translate
 *
 * Multiply the active matrix by a translation matrix.
 * This implements: https://www.opengl.org/sdk/docs/man2/xhtml/glTranslate.xml
 */
void canvashdl::translate(vec3f direction)
{
	// TODO Assignment 1: Multiply the active matrix by a translation matrix.
    mat4f projection(1., 0., 0., direction[0],
                     0., 1., 0., direction[1],
                     0., 0., 1., direction[2],
                     0., 0., 0., 1.);
    
    matrices[active_matrix] *= projection;
}

/* scale
 *
 * Multiply the active matrix by a scaling matrix.
 * This implements: https://www.opengl.org/sdk/docs/man2/xhtml/glScale.xml
 */
void canvashdl::scale(vec3f size)
{
	// TODO Assignment 1: Multiply the active matrix by a scaling matrix.
    mat4f projection(size[0], 0., 0., 0.,
                     0., size[1], 0., 0.,
                     0., 0., size[2], 0.,
                     0., 0., 0., 1.);
    
    matrices[active_matrix] *= projection;
}

/* perspective
 *
 * Multiply the active matrix by a perspective projection matrix.
 * This implements: https://www.opengl.org/sdk/docs/man2/xhtml/gluPerspective.xml
 */
void canvashdl::perspective(float fovy, float aspect, float n, float f)
{
	// TODO Assignment 1: Multiply the active matrix by a perspective projection matrix.
    float fov = tanf(M_PI_2 - fovy/2.);
    
    mat4f projection(fov/aspect, 0., 0., 0.,
                     0., fov, 0., 0.,
                     0., 0., (f+n)/(n-f), (2.*f*n)/(n-f),
                     0., 0., -1., 0.);
    
    matrices[active_matrix] *= projection;
}

/* frustum
 *
 * Multiply the active matrix by a frustum projection matrix.
 * This implements: https://www.opengl.org/sdk/docs/man2/xhtml/glFrustum.xml
 */
void canvashdl::frustum(float l, float r, float b, float t, float n, float f)
{
	// TODO Assignment 1: Multiply the active matrix by a frustum projection matrix.
    mat4f projection(2.*n/(r-l), 0., (r+l)/(r-l), 0.,
                     0., 2.*n/(t-b), (t+b)/(t-b), 0.,
                     0., 0., -(f+n)/(f-n), -2.*f*n/(f-n),
                     0., 0., -1., 0.);
    
    matrices[active_matrix] *= projection;
}

/* ortho
 *
 * Multiply the active matrix by an orthographic projection matrix.
 * This implements: https://www.opengl.org/sdk/docs/man2/xhtml/glOrtho.xml
 */
void canvashdl::ortho(float l, float r, float b, float t, float n, float f)
{
	// TODO Assignment 1: Multiply the active matrix by an orthographic projection matrix.
    mat4f projection(2./(r-l), 0., 0., -(r+l)/(r-l),
                     0., 2./(t-b), 0., -(t+b)/(t-b),
                     0., 0., -2./(f-n), -(f+n)/(f-n),
                     0., 0., 0., 1.);
    
    matrices[active_matrix] *= projection;
}

void canvashdl::viewport(int left, int bottom, int right, int top)
{
	matrices[viewport_matrix] = mat4f((float)(right - left)/2.0, 0.0, 0.0, (float)(right + left)/2.0,
									  0.0, (float)(top - bottom)/2.0, 0.0, (float)(top + bottom)/2.0,
									  0.0, 0.0, (float)depth/2.0, (float)depth/2.0,
									  0.0, 0.0, 0.0, 1.0);

	resize(right - left, top - bottom);
}

/* look_at
 *
 * Move and orient the modelview so the camera is at the 'at' position focused on the 'eye'
 * position and rotated so the 'up' vector is up
 * This implements: https://www.opengl.org/sdk/docs/man2/xhtml/gluLookAt.xml
 */
void canvashdl::look_at(vec3f eye, vec3f at, vec3f up)
{
	// TODO Assignment 1: Emulate the functionality of gluLookAt
    vec3f f = norm(at - eye);
    vec3f s = cross(f, norm(up));
    vec3f u = cross(norm(s), f);
    
    mat4f projection(s[0], s[1], s[2], 0.,
                     u[0], u[1], u[2], 0.,
                     -f[0], -f[1], -f[2], 0.,
                     0., 0., 0., 1.);
    
    matrices[active_matrix] *= projection;
    translate(-eye);
    
}

void canvashdl::update_normal_matrix()
{
	// TODO Assignment 2: calculate the normal matrix
    matrices[normal_matrix] = transpose(inverse(matrices[modelview_matrix]));
}

/* to_window
 *
 * Pixel coordinates to window coordinates.
 */
vec3f canvashdl::to_window(vec2i pixel)
{
	/* TODO Assignment 1: Given a pixel coordinate (x from 0 to width and y from 0 to height),
	 * convert it into window coordinates (x from -1 to 1 and y from -1 to 1).
	 */
    vec3f result(2.0*(float)pixel[0]/(float)(width-1) - 1.0,
                 2.0*(float)(height - 1.0 - pixel[1])/(float)(height-1.0) - 1.0,
                 1.0);
    
    return result;
}

/* to_pixel
 *
 * Window coordinates to pixel coordinates.
 */
vec3i canvashdl::to_pixel(vec3f window)
{
    int x = roundf(width*(window[0]+1.)/2.);
    int y = roundf(height*(window[1]+1.)/2.);
    int z = roundf(depth*(window[2]+1.)/2.);
    
    return vec3i(x,y,z);
}

/* unproject
 *
 * Unproject a window coordinate into world coordinates.
 * This implements: https://www.opengl.org/sdk/docs/man2/xhtml/gluUnProject.xml
 */
vec3f canvashdl::unproject(vec3f window)
{
	// TODO Assignment 1: Unproject a window coordinate into world coordinates.
    return inverse(matrices[modelview_matrix])*inverse(matrices[projection_matrix])*homogenize(window);
}

/* shade_vertex
 *
 * This is the vertex shader.
 * v[0] to v[2] is position
 * v[3] to v[5] is normal
 * v[7] to v[8] is texture coordinates
 * The result from this function is interpolated and passed on to the fragment shader
 * (its also used to draw the geometry during rasterization)
 * Note that the only requirements for the returned values are that the first 3 components
 * be a projected position. The rest are yours to decide what to do with.
 */
vec3f canvashdl::shade_vertex(vec8f v, vector<float> &varying)
{
	// TODO Assignment 1: Do all of the necessary transformations (normal, projection, modelview, etc)
    
	/* TODO Assignment 2: Get the material from the list of uniform variables and
	 * call its vertex shader.
	 */
    
    const materialhdl *material;
    get_uniform("material", material);
    
    if (!material) {
        material = new uniformhdl;
        uniform["material"] = material;
    }
    
    vec4f eye_space_vertex;
    eye_space_vertex = material->shade_vertex(this, vec3f(v[0],v[1],v[2]), vec3f(v[3],v[4],v[5]), varying);
    return eye_space_vertex;

}

/* shade_fragment
 *
 * This is the fragment shader. The pixel color is determined here.
 * the values for v are the interpolated result of whatever you returned from the vertex shader
 */
vec3f canvashdl::shade_fragment(vector<float> varying)
{
	// TODO Assignment 1: Pick a color, any color (as long as it is distinguishable from the background color).

	/* TODO Assignment 2: Get the material from the list of uniform variables and
	 * call its fragment shader.
	 */
    const materialhdl *material;
    get_uniform("material", material);
    
    vec3f color;
    color = material->shade_fragment(this, varying);
    color *= 255.;
    
	return color;
}

/* plot
 *
 * Plot a pixel and check it against the depth buffer.
 */
void canvashdl::plot(vec3i xyz, vector<float> varying)
{
	// TODO Assignment 1: Plot a pixel, calling the fragment shader.
    if ((xyz[0] >= 0 && xyz[0] < width) && (xyz[1] >=0 && xyz[1] < height)) {
        
        /* TODO Assignment 2: Compare the z value against the depth buffer and
         * only render if its less. Then set the depth buffer.
         */
        if (xyz[2] < depth_buffer[width*xyz[1]+xyz[0]]){
            int index = (width*xyz[1]+xyz[0]);
            
            vec3f color = shade_fragment(varying);
            color_buffer[3*index] = color[red];
            color_buffer[3*index+1] = color[green];
            color_buffer[3*index+2] = color[blue];
            
            // Set depth buffer
            depth_buffer[index] = xyz[2];
        }
    }
}

/* plot_point
 *
 * Plot a point given in pixel coordinates.
 */
void canvashdl::plot_point(vec3i v, vector<float> varying)
{
	// TODO Assignment 1: Plot a point given in window coordinates.
    plot(v,varying);
}

/* plot_line
 *
 * Plot a line defined by two points in pixel coordinates.
 */
void canvashdl::plot_line(vec3i vp1, vector<float> v1_varying, vec3i vp2, vector<float> v2_varying)
{
//	// TODO Assignment 1: Implement Bresenham's Algorithm.
//    // Convert to Pixel coordinates here
////    vec3i vp1 = to_pixel(v1);
////    vec3i vp2 = to_pixel(v2);
//    
//    // Variables
//    vec3i xy, tmp;
//    vector<float> v, tmpv;
//    int dy = abs(vp2[1] - vp1[1]);
//    int dx = abs(vp2[0] - vp1[0]);
//    
//    // Check cases
//    // abs(slope) < 1
//    if(dy<=dx){
//        // orient points for E/NE/SE movement
//        if(vp2[0]>=vp1[0]){
//            xy = vp1;
//            v = v1_varying;
//
//        } else {
//            xy = vp2;
//            tmp = vp2;
//            vp2 = vp1;
//            vp1 = tmp;
//            
//            v = v2_varying;
//            tmpv = v2_varying;
//            v2_varying = v1_varying;
//            v1_varying = tmpv;
//        }
//        
//        plot(xy, v);
//        int d = 2*dy-dx;
//        // step through each pixel
//        while (xy[0] <= vp2[0]){
//            xy[0]++;
//            // E
//            if(d < 0){
//                d+=2*dy;
//            }
//            // NE/SE
//            else {
//                if(vp2[1]>xy[1]){
//                    xy[1]++;
//                } else {
//                    xy[1]--;
//                }
//                d+=2*(dy-dx);
//            }
//            
//            // Interpolate varying
//            float t12 = (float)(vp2[0] - xy[0])/dx;
//            xy[2] = (int)(t12*vp1[2] + (1.-t12)*vp2[2]);
//            for (int j = 0; j < v1_varying.size(); j++) {
//                v[j] = t12*v1_varying[j] + (1.-t12)*v2_varying[j];
//            }
//
//            plot(xy, v);
//        }
//    }
//    // abs(slope) > 1
//    else {
//        // orient points for E/NE/SE movement
//        if(vp2[1]>=vp1[1]) {
//            xy = vp1;
//            v = v1_varying;
//            
//        } else {
//            xy = vp2;
//            tmp = vp2;
//            vp2 = vp1;
//            vp1 = tmp;
//            
//            v = v2_varying;
//            tmpv = v2_varying;
//            v2_varying = v1_varying;
//            v1_varying = tmpv;
//        }
//        
//        plot(xy, v);
//        int d = 2*dx-dy;
//        // step through each pixel
//        while (xy[1] <= vp2[1]){
//            xy[1]++;
//            // E
//            if(d<=0){
//                d+=2*dx;
//            }
//            // NE/SE
//            else {
//                if(vp2[0]>xy[0]){
//                    xy[0]++;
//                } else {
//                    xy[0]--;
//                }
//                d+=2*(dx-dy);
//            }
//            
//            // Interpolate varying
//            float t12 = (float)(vp2[1] - xy[1])/dy;
//            xy[2] = (int)(t12*vp1[2] + (1.-t12)*vp2[2]);
//            for (int j = 0; j < v1_varying.size(); j++) {
//                v[j] = t12*v1_varying[j] + (1.-t12)*v2_varying[j];
//            }
//            
//            plot(xy, v);
//        }
//    }

	// TODO Assignment 2: Interpolate the varying values before passing them into plot.
    
//    vec3i s1 = (vec3i)v1;
//    vec3i s2 = (vec3i)v2;
    vec3i s1 = vp1;
    vec3i s2 = vp2;
    
    int b = (abs(s2[1] - s1[1]) > abs(s2[0] - s1[0]));
    
    vec2i dv = (vec2i)s2 - (vec2i)s1;
    
    vec2i step((int)(dv[0] > 0) - (int)(dv[0] < 0),
               (int)(dv[1] > 0) - (int)(dv[1] < 0));
    dv[0] *= step[0];
    dv[1] *= step[1];
    
    int D = 2*dv[1-b] - dv[b];
    
    // TODO Assignment 2: Interpolate the varying values before passing them into plot.
    vector<float> varying = v1_varying;
    
    plot(s1, varying);
    
    vec3i p = s1;
    float increment = (float)step[b]/(float)(s2[b] - s1[b]);
    float interpolate = 0.0f;
    float zdiff = vp2[2] - vp1[2];
    p[b]+=step[b];
    while (step[b]*p[b] < step[b]*s2[b])
    {
        if (D > 0)
        {
            p[1-b] += step[1-b];
            D += 2*(dv[1-b] - dv[b]);
        }
        else
            D += 2*dv[1-b];
        
        p[2] = (int)(zdiff*interpolate) + s1[2];
        
        // Interpolate varying
        for (int j = 0; j < varying.size(); j++) {
            varying[j] = (float)(v2_varying[j] - v1_varying[j])*interpolate + v1_varying[j];
        }

        
        plot(p, varying);
        p[b] += step[b];
        interpolate += increment;
    }
}

void canvashdl::plot_horizontal_line(vec3i vp1, vector<float> v1_varying, vec3i vp2, vector<float> v2_varying) {
    
    // Draw from right to left
    if (vp1[0] > vp2[0]) {
        vec3i temp = vp1;
        vp1 = vp2;
        vp2 = temp;
        
        vector<float> tmpV = v1_varying;
        v1_varying = v2_varying;
        v2_varying = tmpV;
    }
    
    int dx = vp2[0] - vp1[0];

    vec3i xy = vp1;
    vector<float> v = v1_varying;
    float t123;
    
    for (int i = 0; i < dx; i++) {
        t123 = (float)i/(float)dx;
        xy[2] = (int)(t123*vp2[2] + (1.-t123)*vp1[2]);
        for (int j = 0; j < v1_varying.size(); j++) {
            v[j] = t123*v2_varying[j] + (1.-t123)*v1_varying[j];
        }
        plot(xy, v);
        xy[0]++;
    }
    
    plot(vp2, v2_varying);
}

int canvashdl::sign(int x) {
    if (x > 0) {
        return 1;
    } else if (x < 0) {
        return -1;
    } else {
        return 0;
    }
}

/* plot_half_triangle
 *
 * Plot half of a triangle defined by three points in window coordinates (v1, v2, v3).
 * The remaining inputs are as follows: (s1, s2, s3) are the pixel coordinates of (v1, v2, v3),
 * and (ave) is the average value of the normal and texture coordinates for flat shading.
 * Use Bresenham's algorithm for this. You may plot the horizontal half or the vertical half.
 */
void canvashdl::plot_half_triangle(vec3i s1, vector<float> v1_varying, vec3i s2, vector<float> v2_varying, vec3i s3, vector<float> v3_varying, vector<float> ave_varying)
{
	// TODO Assignment 2: Implement Bresenham's half triangle fill algorithm (refer Ned's link on Piazza)
    
    // Interpolate z-value for every pixel to be plotted and call plot function (z-buffer algorithm)
    
	// TODO Assignment 2: Interpolate the varying values before passing them into plot.
    
    // Variables
    vec3i t2 = s1, t3 = s1;
    vector<float> t2_varying, t3_varying; // TODO: interpolate
    float t12 = 0., t13 = 0.;
    
    int dy2 = abs(s2[1] - s1[1]);
    int dy2_sign = sign(s2[1] - s1[1]);
    int dx2 = abs(s2[0] - s1[0]);
    int dx2_sign = sign(s2[0] - s1[0]);
    
    int dy3 = abs(s3[1] - s1[1]);
    int dy3_sign = sign(s3[1] - s1[1]);
    int dx3 = abs(s3[0] - s1[0]);
    int dx3_sign = sign(s3[0] - s1[0]);
    
    if (shade_model == flat || shade_model == none) {
        t2_varying = ave_varying;
        t3_varying = ave_varying;
        plot_line(s1, ave_varying, s2, ave_varying);
        plot_line(s2, ave_varying, s3, ave_varying);
        plot_line(s3, ave_varying, s1, ave_varying);
    } else {
        t2_varying = v1_varying;
        t3_varying = v1_varying;
        plot_line(s1, v1_varying, s2, v2_varying);
        plot_line(s2, v2_varying, s3, v3_varying);
        plot_line(s3, v3_varying, s1, v1_varying);
    }
    
    // Swap values based on if slope > 1
    bool swap2 = false, swap3 = false;
    
    if (dy2 > dx2)
    {
        int tmp = dx2;
        dx2 = dy2;
        dy2 = tmp;
        swap2 = true;
    }
    
    if (dy3 > dx3)
    {
        int tmp = dx3;
        dx3 = dy3;
        dy3 = tmp;
        swap3 = true;
    }
    
    int e2 = 2 * dy2 - dx2;
    int e3 = 2 * dy3 - dx3;
    
    plot(s1, v1_varying);

    for (int i = 0; i < dx2; i++)
    {
        // Trace next value of t2
        while (e2 >= 0)
        {
            if (swap2)
                t2[0] += dx2_sign;
            else
                t2[1] += dy2_sign;
            e2 -= 2 * dx2;
        }
        
        if (swap2)
            t2[1] += dy2_sign;
        else
            t2[0] += dx2_sign;
        
        e2 += 2 * dy2;
        
        // Trace next value of t3
        while (t3[1] != t2[1])
        {
            while (e3 >= 0)
            {
                if (swap3)
                    t3[0] += dx3_sign;
                else
                    t3[1] += dy3_sign;
                e3 -= 2 * dx3;
            }
            
            if (swap3)
                t3[1] += dy3_sign;
            else
                t3[0] += dx3_sign;
            
            e3 += 2 * dy3;
        }
        
        // Interpolate Z values
        t12 = (float)(s2[1] - t2[1])/(float)(s2[1] - s1[1]);
        t13 = (float)(s3[1] - t3[1])/(float)(s3[1] - s1[1]);
        
        t2[2] = t12*s1[2] + (1.-t12)*s2[2];
        t3[2] = t13*s1[2] + (1.-t13)*s3[2];
        
        // Plot based on shading model
        if (shade_model == gouraud || shade_model == phong) {
            // Interpolate varying
            for (int j = 0; j < t2_varying.size(); j++) {
                t2_varying[j] = t12*v1_varying[j] + (1.-t12)*v2_varying[j];
                t3_varying[j] = t13*v1_varying[j] + (1.-t13)*v3_varying[j];
            }
        }
        // Plot line between t2 and t3
        plot_horizontal_line(t2, t2_varying, t3, t3_varying);
    }
}


/* plot_triangle
 *
 * Plot a triangle. (v1, v2, v3) are given in window coordinates.
 */
void canvashdl::plot_triangle(vec3f v1, vector<float> v1_varying, vec3f v2, vector<float> v2_varying, vec3f v3, vector<float> v3_varying)
{
	/* TODO Assignment 1: Use the above functions to plot a whole triangle. Don't forget to
	 * take into account the polygon mode. You should be able to render the
	 * triangle as 3 points or 3 lines.
	 */
    
    vec3i vi1 = to_pixel(v1), vi2 = to_pixel(v2), vi3 = to_pixel(v3);
    
    switch (polygon_mode) {
        case point: {
            // 3 Point implementation
            plot_point(vi1, v1_varying);
            plot_point(vi2, v2_varying);
            plot_point(vi3, v3_varying);
            break;
        }
        case line: {
            // 3 Line implementation
            plot_line(vi1, v1_varying, vi2, v2_varying);
            plot_line(vi2, v2_varying, vi3, v3_varying);
            plot_line(vi3, v3_varying, vi1, v1_varying);
            break;
        }
        case fill: {
            
            // TODO Assignment 2: Calculate the average varying vector for flat shading and call plot_half_triangle as needed.
            
            // Calculate pixel coordinates, retain z-value
            vec3i temp;
            vector<float> tempV;
            
            // Sort by Y
            if (vi1[1] > vi2[1])
            {
                temp = vi1;
                vi1 = vi2;
                vi2 = temp;
                
                tempV = v1_varying;
                v1_varying = v2_varying;
                v2_varying = tempV;
            }
            if (vi1[1] > vi3[1])
            {
                temp = vi1;
                vi1 = vi3;
                vi3 = temp;
                
                tempV = v1_varying;
                v1_varying = v3_varying;
                v3_varying = tempV;
            }
            if (vi2[1] > vi3[1])
            {
                temp = vi2;
                vi2 = vi3;
                vi3 = temp;
                
                tempV = v2_varying;
                v2_varying = v3_varying;
                v3_varying = tempV;
            }
            
            // Calculate average varying vector
            vector<float> ave_varying(v1_varying.size(), 0);
            for (int i=0; i < ave_varying.size(); i++){
                ave_varying[i] = (v1_varying[i] + v2_varying[i] + v3_varying[i])/3.;
            }
            
            // bottom-flat triangle
            if (vi2[1] == vi3[1]) {
                plot_half_triangle(vi1, v1_varying, vi2, v2_varying, vi3, v3_varying, ave_varying);

            } else if (vi1[1] == vi2[1]){  // top-flat triangle
                plot_half_triangle(vi3, v3_varying, vi1, v1_varying, vi2, v2_varying, ave_varying);

            } else {
                // Divide triangle into 2 flat sided triangles and call plot_half_triangle on each one
                int x = (int)(vi1[0] + ((float)(vi2[1] - vi1[1]) / (float)(vi3[1] - vi1[1])) * (vi3[0] - vi1[0]));
                int y = vi2[1];
                
                // Interpolate Z
                float t13 = (float)(vi3[1] - y)/(float)(vi3[1] - vi1[1]);
                int z = (int)(t13*vi1[2] + (1.-t13)*vi3[2]);

                // Interpolate Varying
                vector<float> v4_varying = v2_varying;
                for (int j = 0; j < v4_varying.size(); j++) {
                    v4_varying[j] = t13*v1_varying[j] + (1.-t13)*v3_varying[j];
                }
                
                // Plot top and bottom triangles
                vec3i vi4  (x, y, z);
                plot_half_triangle(vi1, v1_varying, vi2, v2_varying, vi4, v4_varying, ave_varying);
                plot_half_triangle(vi3, v3_varying, vi2, v2_varying, vi4, v4_varying, ave_varying);
            }
            
            break;
        }
        default:
            break;
    }
}

/* draw_points
 *
 * Draw a set of 3D points on the canvas. Each point in geometry is
 * formatted (vx, vy, vz, nx, ny, nz, s, t).
 */
void canvashdl::draw_points(const vector<vec8f> &geometry)
{
    // TODO Assignment 2: Update the normal matrix
    update_normal_matrix();
    
	// TODO Assignment 1: Clip the points against the frustum, call the vertex shader, and then draw them.
    vector<vec8f> new_geometry = geometry;
    
    for (vector<vec8f>::iterator iter = new_geometry.begin(); iter != new_geometry.end(); ++iter) {
        vector<float> varying;
        *iter = shade_vertex(*iter, varying);
        plot_point(to_pixel(*iter), varying);
    }
}

/* Draw a set of 3D lines on the canvas. Each point in geometry
 * is formatted (vx, vy, vz, nx, ny, nz, s, t).
 */
void canvashdl::draw_lines(const vector<vec8f> &geometry, const vector<int> &indices)
{
    // TODO Assignment 2: Update the normal matrix
    update_normal_matrix();
    
	// TODO Assignment 1: Clip the lines against the frustum, call the vertex shader, and then draw them.
    construct_planes();
    vector<vec8f> new_geometry;
    vector<vector<float>> new_varying;
    
    for (int i = 0; i < indices.size(); i+= 2) {
        
        // Clip line
        vector<vec8f> new_points = clip_line(geometry[indices[i]], geometry[indices[i+1]]);
        
//        if (!new_points.empty()){
        
            // Vertex shader
            for (vector<vec8f>::iterator iter = new_points.begin(); iter != new_points.end(); ++iter) {
                
                vector<float> varying;
                *iter = shade_vertex(*iter, varying);
                new_geometry.push_back(*iter);
                new_varying.push_back(varying);
            }
//        }
    }
    
    // Plot lines
    for (int i = 0; i < new_geometry.size(); i+=2) {
        plot_line(to_pixel(new_geometry[i]), new_varying[i], to_pixel(new_geometry[i+1]), new_varying[i+1]);
    }
}

/* Draw a set of 3D triangles on the canvas. Each point in geometry is
 * formatted (vx, vy, vz, nx, ny, nz, s, t). Don't forget to clip the
 * triangles against the clipping planes of the projection. You can't
 * just not render them because you'll get some weird popping at the
 * edge of the view. Also, this is where front/back face culling is implemented.
 */
void canvashdl::draw_triangles(const vector<vec8f> &geometry, const vector<int> &indices)
{
    // TODO Assignment 2: Update the normal matrix
    update_normal_matrix();
    
	/* TODO Assignment 1: Clip the triangles against the frustum, call the vertex shader,
	 * break the resulting polygons back into triangles, implement front and back face
	 * culling, and then draw the remaining triangles.
	 */
    construct_planes();
    vector<vec8f> new_geometry;
    vector<int> new_indices;
    vector<vector<float>> new_varying;
    
    for (int i = 2; i < indices.size(); i += 3) {
        
        // Clip triangles
        vector<vec8f> new_points = {geometry[indices[i-2]], geometry[indices[i-1]], geometry[indices[i]]};
        
        for (int i = 0; i < planes.capacity(); i++) {
            new_points = clip_triangle_against_plane (new_points, planes[i]);
            
            if (new_points.size() == 0) {
                break;
            }
        }
        
        if (new_points.size()){
        
            // Form new triangles (use fanning structure)
            for (int j = 0; j < new_points.size() - 2; j++){
                new_indices.push_back((int)new_geometry.size());
                new_indices.push_back((int)new_geometry.size() + j + 1);
                new_indices.push_back((int)new_geometry.size() + j + 2);
            }
            
            // Vertex shader
            for (vector<vec8f>::iterator iter = new_points.begin(); iter != new_points.end(); ++iter) {
                
                vector<float> varying;
                *iter = shade_vertex(*iter, varying);
                new_geometry.push_back(*iter);
                new_varying.push_back(varying);
            }
        }
    }
    
    // Do culling and plot triangles
    for (int i = 2; i < new_indices.size(); i+=3)
    {
        vec3f v1 = new_geometry[new_indices[i-2]];
        vec3f v2 = new_geometry[new_indices[i-1]];
        vec3f v3 = new_geometry[new_indices[i]];

        vec3f normal = cross(norm(v3 - v2), norm(v2 - v1));
        
        if (culling_mode == disable || (normal[2] >= 0.0 && culling_mode == backface) || (normal[2] <= 0.0 && culling_mode == frontface))
        {
            plot_triangle(v1, new_varying[new_indices[i-2]], v2, new_varying[new_indices[i-1]], v3, new_varying[new_indices[i]]);
        }
    }
}

//vector<vec8f> canvashdl::clip_triangle(vec8f v1, vec8f v2, vec8f v3)
//{
//    vector<vec8f> clipped_points = {v1,v2,v3};
//    
//    for (int i = 0; i < planes.capacity(); i++) {
//        clipped_points = clip_triangle_against_plane (clipped_points, planes[i]);
//        
//        if (clipped_points.size() == 0) {
//            break;
//        }
//    }
//    
//    return clipped_points;
//}

// clips a triangle against a single plane
vector<vec8f> canvashdl::clip_triangle_against_plane (vector<vec8f> triangle_points, plane clipping_plane) {
    
    vector<vec8f> new_clipping_points;
    
    for (int i = 0; i < triangle_points.size(); i++) {
        //clip_line_against_plane (triangle_points[i], triangle_points[(i+1)%triangle_points.size()], clipping_plane, new_clipping_points);
        vec8f v1 = triangle_points[i];
        vec8f v2 = triangle_points[(i+1)%triangle_points.size()];
        vec3f v1_coords = {v1[0], v1[1], v1[2]};
        vec3f v2_coords = {v2[0], v2[1], v2[2]};
        
        float d1 = dot(v1_coords, clipping_plane.normal) + clipping_plane.distance;
        float d2 = dot(v2_coords, clipping_plane.normal) + clipping_plane.distance;
        
        if (d1 < 0 && d2 >= 0){ // Outside to inside (first point outside and second point inside)
            
            float s = d2/(d2 - d1);
            vec3f intersection_point = get_intersection_point (v2_coords, v1_coords, s);
            v1.set(0, 3, intersection_point);
            new_clipping_points.push_back(v1);
            new_clipping_points.push_back(v2);
            
        } else if (d1 >= 0 && d2 < 0) { // Inside to outside (first point inside and second point outside)
            
            float s = d1/(d1 - d2);
            vec3f intersection_point = get_intersection_point (v1_coords, v2_coords, s);
            //v1_coords = intersection_point;
            v1.set(0, 3, intersection_point);
            new_clipping_points.push_back(v1);
            
        } else if (d1 >= 0 && d2>= 0){ // Both points inside plane
            new_clipping_points.push_back(v2);
        }
    }
    return new_clipping_points;
}

//vector<vec8f> canvashdl::clip_triangle(vec8f v1, vec8f v2, vec8f v3)
//{
//    vector<vec8f> clipped_points = {v1,v2,v3};
//    
//    for (int i = 0; i < planes.capacity(); i++) {
//        //clipped_points = clip_triangle_against_plane (clipped_points, planes[i]);
//        vector<vec8f> new_clipping_points;
//        
//        for (int j = 0; j < clipped_points.size(); j++) {
//            //clip_line_against_plane (clipped_points[j], clipped_points[(j+1)%clipped_points.size()], planes[i], new_clipping_points);
//            vec8f v1 = clipped_points[j];
//            vec8f v2 = clipped_points[(j+1)%clipped_points.size()];
//            vec3f v1_coords = {v1[0], v1[1], v1[2]};
//            vec3f v2_coords = {v2[0], v2[1], v3[2]};
//            
//            float d1 = dot(v1_coords, planes[i].normal) + planes[i].distance;
//            float d2 = dot(v2_coords, planes[i].normal) + planes[i].distance;
//            
//            if (d1 < 0 && d2 >= 0){ // Outside to inside (first point outside and second point inside)
//                
//                float s = d2/(d2 - d1);
//                vec3f intersection_point = get_intersection_point (v2_coords, v1_coords, s);
//                v1.set(0, 3, intersection_point);
//                new_clipping_points.push_back(v1);
//                new_clipping_points.push_back(v2);
//                
//                //cout << "Outside to inside" << endl;
//                
//            } else if (d1 >= 0 && d2 < 0) { // Inside to outside (first point inside and second point outside)
//                
//                float s = d1/(d1 - d2);
//                vec3f intersection_point = get_intersection_point (v1_coords, v2_coords, s);
//                v1.set(0, 3, intersection_point);
//                new_clipping_points.push_back(v1);
//                
//                //cout << "Inside to outside" << endl;
//                
//            } else if (d1 >= 0 && d2>= 0){ // Both points inside plane
//                new_clipping_points.push_back(v2);
//                
//                //cout << "Both inside" << endl;
//            }
//        }
//        clipped_points = new_clipping_points;
//        if (clipped_points.size() == 0) {
//            break;
//        }
//    }
//    
//    return clipped_points;
//}

// Clip given line with all 6 frustum planes and return final points
vector<vec8f> canvashdl::clip_line(vec8f point1, vec8f point2)
{
    vector<vec8f> clipped_points;
    vec3f point1_coords = {point1[0], point1[1], point1[2]};
    vec3f point2_coords = {point2[0], point2[1], point2[2]};
    
    for (int i = 0; i < planes.capacity(); i ++){
        float d1 = dot(point1_coords, planes[i].normal) + planes[i].distance;
        float d2 = dot(point2_coords, planes[i].normal) + planes[i].distance;
        
        if (d1 < 0 && d2 >= 0){
            
            float s = d2/(d2 - d1);
            vec3f intersection_point = get_intersection_point (point2_coords, point1_coords, s);
            point1_coords = intersection_point;
            
        } else if (d1 >= 0 && d2 < 0) {
            
            float s = d1/(d1 - d2);
            vec3f intersection_point = get_intersection_point (point1_coords, point2_coords, s);
            point2_coords = intersection_point;
            
        } else if (d1 < 0 && d2 < 0){
            clipped_points.clear();
            return clipped_points;
        }
    }
    
    point1.set(0, 3, point1_coords);
    point2.set(0, 3, point2_coords);
    clipped_points.push_back(point1);
    clipped_points.push_back(point2);
    
    return clipped_points;
}

void canvashdl::construct_planes ()
{
//    plane p;                      // plane to construct from a,b and c
//    
//    // build normal vector
//    vec3f q,v;
//    q[0] = p2[0] - p1[0];    v[0] = p2[0] - p3[0];
//    q[1] = p2[1] - p1[1];    v[1] = p2[1] - p3[1];
//    q[2] = p2[2] - p1[2];    v[2] = p2[2] - p3[2];
//    p.normal = cross (q,v);
//    p.normal = norm (p.normal);
//    
//    // calculate distance to origin
//    p.distance = dot (p.normal, p1);
//    
//    return p;
    
    mat4f a = matrices[projection_matrix]*matrices[modelview_matrix];
    
    planes[0] = get_plane (a[0], a[3]); // left
    planes[1] = get_plane (-a[0], a[3]); // right
    planes[2] = get_plane (a[1], a[3]);  // bottom
    planes[3] = get_plane (-a[1], a[3]);  // top
    planes[4] = get_plane (a[2], a[3]);  // near
    planes[5] = get_plane (-a[2], a[3]);  // far
    
    //cout << planes[5].normal[2] << endl;
}

canvashdl::plane canvashdl::get_plane (vec<float,4> row1, vec<float,4> row2){
    
    plane new_plane;
    vec3f normal;
    float distance;
    
    normal[0] = row1[0] + row2[0];
    normal[1] = row1[1] + row2[1];
    normal[2] = row1[2] + row2[2];
    distance = row1[3] + row2[3];
    
    new_plane.normal = normal;
    new_plane.distance = distance;
    
    return new_plane;
}

vec3f canvashdl::get_intersection_point(vec3f inside_point, vec3f outside_point, float intersection_factor) {
    vec3f intersection_point;
    
    intersection_point[0] = inside_point[0] + intersection_factor*(outside_point[0] - inside_point[0]);
    intersection_point[1] = inside_point[1] + intersection_factor*(outside_point[1] - inside_point[1]);
    intersection_point[2] = inside_point[2] + intersection_factor*(outside_point[2] - inside_point[2]);
    
    return intersection_point;

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Do not edit anything below here, that code just sets up OpenGL to render a single
 * quad that covers the whole screen, applies the color_buffer as a texture to it, and
 * keeps the buffer size and texture up to date.
 */
void canvashdl::load_texture()
{
	glGenTextures(1, &screen_texture);
	check_error(__FILE__, __LINE__);
	glActiveTexture(GL_TEXTURE0);
	check_error(__FILE__, __LINE__);
	glBindTexture(GL_TEXTURE_2D, screen_texture);
	check_error(__FILE__, __LINE__);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	check_error(__FILE__, __LINE__);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	check_error(__FILE__, __LINE__);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	check_error(__FILE__, __LINE__);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	check_error(__FILE__, __LINE__);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	check_error(__FILE__, __LINE__);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, color_buffer);
	check_error(__FILE__, __LINE__);
}

void canvashdl::load_geometry()
{
	// x, y, s, t
	const GLfloat geometry[] = {
			-1.0, -1.0, 0.0, 0.0,
			 1.0, -1.0, 1.0, 0.0,
			-1.0,  1.0, 0.0, 1.0,
			-1.0,  1.0, 0.0, 1.0,
			 1.0, -1.0, 1.0, 0.0,
			 1.0,  1.0, 1.0, 1.0
	};

	glGenBuffers(1, &screen_geometry);
	glBindBuffer(GL_ARRAY_BUFFER, screen_geometry);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*4*6, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat)*4*6, geometry);
}

void canvashdl::load_shader()
{
	GLuint vertex = load_shader_file(working_directory + "res/canvas.vx", GL_VERTEX_SHADER);
	GLuint fragment = load_shader_file(working_directory + "res/canvas.ft", GL_FRAGMENT_SHADER);

	screen_shader = glCreateProgram();
	glAttachShader(screen_shader, vertex);
	glAttachShader(screen_shader, fragment);
	glLinkProgram(screen_shader);
}

void canvashdl::init_opengl()
{
	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, width, height);

	load_texture();
	load_geometry();
	load_shader();
	initialized = true;
}

void canvashdl::check_error(const char *file, int line)
{
	GLenum error_code;
	error_code = glGetError();
	if (error_code != GL_NO_ERROR)
		cerr << "error: " << file << ":" << line << ": " << gluErrorString(error_code) << endl;
}

double canvashdl::get_time()
{
	timeval gtime;
	gettimeofday(&gtime, NULL);
	return gtime.tv_sec + gtime.tv_usec*1.0E-6;
}

void canvashdl::resize(int w, int h)
{
	glViewport(0, 0, w, h);
	last_reshape_time = get_time();
	reshape_width = w;
	reshape_height = h;
}

void canvashdl::swap_buffers()
{
	if (!initialized)
		init_opengl();

	if (last_reshape_time > 0.0 && get_time() - last_reshape_time > 0.125)
		resize(reshape_width, reshape_height);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(screen_shader);
	check_error(__FILE__, __LINE__);

	glActiveTexture(GL_TEXTURE0);
	check_error(__FILE__, __LINE__);
	glBindTexture(GL_TEXTURE_2D, screen_texture);
	check_error(__FILE__, __LINE__);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, color_buffer);
	check_error(__FILE__, __LINE__);
	glUniform1i(glGetUniformLocation(screen_shader, "tex"), 0);
	check_error(__FILE__, __LINE__);

	glBindBuffer(GL_ARRAY_BUFFER, screen_geometry);
	check_error(__FILE__, __LINE__);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	check_error(__FILE__, __LINE__);
	glEnableClientState(GL_VERTEX_ARRAY);
	check_error(__FILE__, __LINE__);

	glTexCoordPointer(2, GL_FLOAT, sizeof(GLfloat)*4, (float*)(sizeof(GLfloat)*2));
	check_error(__FILE__, __LINE__);
	glVertexPointer(2, GL_FLOAT, sizeof(GLfloat)*4, NULL);
	check_error(__FILE__, __LINE__);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	check_error(__FILE__, __LINE__);

	glDisableClientState(GL_VERTEX_ARRAY);
	check_error(__FILE__, __LINE__);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	check_error(__FILE__, __LINE__);

	glutSwapBuffers();
	check_error(__FILE__, __LINE__);
}

int canvashdl::get_width()
{
	return width;
}

int canvashdl::get_height()
{
	return height;
}
