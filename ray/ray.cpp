#include <stdio.h>
#include <stdlib.h>

#include "udray.h"
#include "glm.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

extern Camera *ray_cam;       // camera info
extern int image_i, image_j;  // current pixel being shaded
extern bool wrote_image;      // has the last pixel been shaded?

extern double dptMin = 0.0f;		// minimum depth
extern double dptMax = 0.0f;		// maximum depth
extern bool firstIntersect = false;
extern GLfloat* dptImage = nullptr;

// reflection/refraction recursion control

extern int maxlevel;          // maximum depth of ray recursion 
extern double minweight;      // minimum fractional contribution to color

// these describe the scene

extern vector < GLMmodel * > model_list;
extern vector < Sphere * > sphere_list;
extern vector < Light * > light_list;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// intersect a ray with the entire scene (.obj models + spheres)

// x, y are in pixel coordinates with (0, 0) the upper-left hand corner of the image.
// color variable is result of this function--it carries back info on how to draw the pixel

void trace_ray(int level, double weight, Ray *ray, Vect color)
{
	Intersection *nearest_inter = NULL;
	Intersection *inter = NULL;
	int i;

	// test for intersection with all .obj models

	for (i = 0; i < model_list.size(); i++) {
		inter = intersect_ray_glm_object(ray, model_list[i]);
		update_nearest_intersection(&inter, &nearest_inter);
	}

	// test for intersection with all spheres

	for (i = 0; i < sphere_list.size(); i++) {
		inter = intersect_ray_sphere(ray, sphere_list[i]);
		update_nearest_intersection(&inter, &nearest_inter);
	}

	// "color" the ray according to intersecting surface properties

	// choose one of the simpler options below to debug or preview your scene more quickly.
	// another way to render faster is to decrease the image size.

	if (nearest_inter) {
		
		// record minimum depth && maximum depth
		if (!firstIntersect)
		{
			dptMin = dptMax = nearest_inter->t;
			firstIntersect = true;
		}

		//shade_ray_false_color_normal(nearest_inter, color);
		//    shade_ray_intersection_mask(color);  
		//shade_ray_diffuse(ray, nearest_inter, color);
		shade_ray_recursive(level, weight, ray, nearest_inter, color);

		// record depth information
		dptImage[image_i + image_j*ray_cam->im->w] = nearest_inter->t;
		if (nearest_inter->t < dptMin)
			dptMin = nearest_inter->t;
		else if (nearest_inter->t > dptMax)
			dptMax = nearest_inter->t;
	}

	// color the ray using a default

	else
	{
		shade_ray_background(ray, color);

		// record depth information
		dptImage[image_i + image_j*ray_cam->im->w] = MAXDEPTH;
	}
}

//----------------------------------------------------------------------------

// test for ray-sphere intersection; return details of intersection if true

Intersection *intersect_ray_sphere(Ray *ray, Sphere *S)
{
	// FILL IN CODE (line below says "no" for all spheres, so replace it)
	Vect deltP;
	double t, deter, dotP_dir_deltP;
	Intersection* inter;

	VectSub(S->P, ray->orig, deltP);
	dotP_dir_deltP = VectDotProd(ray->dir, deltP);

	Vect tmp;
	VectAddS(-1.0f * dotP_dir_deltP, ray->dir, deltP, tmp);
	deter = SQUARE(S->radius) - SQUARE(tmp[X]) - SQUARE(tmp[Y]) - SQUARE(tmp[Z]);

	if (deter < 0.0f)
		return NULL;
	
	else
	{
		deter = sqrt(deter);
		if (dotP_dir_deltP + deter < 0.0f)
			return NULL;

		else
		{
			inter = make_intersection();
			inter->t = (dotP_dir_deltP - deter) <= 0.0f ? (dotP_dir_deltP + deter) : (dotP_dir_deltP - deter);
			VectAddS(inter->t, ray->dir, ray->orig, inter->P);
			inter->surf = S->surf;
			VectSub(inter->P, S->P, inter->N);
			VectUnit(inter->N);
			return inter;
		}
	}
}

//----------------------------------------------------------------------------

// only local, ambient + diffuse L (no specular, shadows, reflections, or refractions)

void shade_ray_diffuse(Ray *ray, Intersection *inter, Vect color)
{
	Vect L;
	double diff_factor;

	// iterate over lights

	for (int i = 0; i < light_list.size(); i++) {

		// AMBIENT

		color[R] += inter->surf->amb[R] * light_list[i]->amb[R];
		color[G] += inter->surf->amb[G] * light_list[i]->amb[G];
		color[B] += inter->surf->amb[B] * light_list[i]->amb[B];

		// DIFFUSE
		Vect lighting;
		VectSub(light_list[i]->P, inter->P, lighting);
		VectUnit(lighting);

		double cosin = VectDotProd(inter->N, lighting);
		if (cosin <= 0.0f)
		{
			cosin = 0.0f;
			continue;
		}

		color[R] += inter->surf->diff[R] * light_list[i]->diff[R] * cosin;
		color[G] += inter->surf->diff[G] * light_list[i]->diff[G] * cosin;
		color[B] += inter->surf->diff[B] * light_list[i]->diff[B] * cosin;
	}	

	// clamp color to [0, 1]
	VectClamp(color, 0, 1);
}

//----------------------------------------------------------------------------

// same as shade_ray_diffuse(), but add specular L + shadow rays (i.e., full Phong illumination model)

void shade_ray_local(Ray *ray, Intersection *inter, Vect color)
{
	// FILL IN CODE 
	for (int i = 0; i < light_list.size(); i++)
	{
		// DIFFUSE
		Vect L;
		VectSub(light_list[i]->P, inter->P, L);
		VectUnit(L);

		double cosin_diff = VectDotProd(inter->N, L);
		if (cosin_diff <= 0.0f)
		{
			cosin_diff = 0.0f;
		}

		// SPECULAR
		Vect reflect;
		VectSub(light_list[i]->P, inter->P, L);
		VectUnit(L);

		VectNegate(L, L);
		VectAddS(VectDotProd(inter->N, L)*-2.0f, inter->N, L, reflect);

		VectUnit(ray->dir);
		VectUnit(reflect);

		double dotProd = VectDotProd(ray->dir, reflect);
		double cosin_spec = 1.0f;

		if (dotProd <= 0.0f)
		{
			cosin_spec = 0.0f;
		}

		else
		{
			for (int i = 0; i < inter->surf->spec_exp; i++)
				cosin_spec *= dotProd;
		}

		color[R] += 
			inter->surf->amb[R] * light_list[i]->amb[R] + 
			inter->surf->diff[R] * light_list[i]->diff[R] * cosin_diff + 
			inter->surf->spec[R] * light_list[i]->spec[R] * cosin_spec;
		
		color[G] += 
			inter->surf->amb[G] * light_list[i]->amb[G] +
			inter->surf->diff[G] * light_list[i]->diff[G] * cosin_diff +
			inter->surf->spec[G] * light_list[i]->spec[G] * cosin_spec;
		
		color[B] += 
			inter->surf->amb[B] * light_list[i]->amb[B] +
			inter->surf->diff[B] * light_list[i]->diff[B] * cosin_diff +
			inter->surf->spec[B] * light_list[i]->spec[B] * cosin_spec;
	}

	VectClamp(color, 0, 1);
}

//----------------------------------------------------------------------------

// full shading model: ambient/diffuse/specular lighting, shadow rays, recursion for reflection, refraction

// level = recursion level (only used for reflection/refraction)

void shade_ray_recursive(int level, double weight, Ray *ray, Intersection *inter, Vect color)
{
	Surface *surf;
	int i;

	// initialize color to Phong reflectance model

	shade_ray_local(ray, inter, color);

	// if not too deep, recurse

	if (level + 1 < maxlevel) {

		// add reflection component to color

		if (surf->reflectivity * weight > minweight) {

			// FILL IN CODE

		}

		// add refraction component to color

		if (surf->transparency * weight > minweight) {

			// GRAD STUDENTS -- FILL IN CODE

		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// ray trace another pixel if the image isn't finished yet

void idle()
{
	if (image_j < ray_cam->im->h) {

		raytrace_one_pixel(image_i, image_j);

		image_i++;

		if (image_i == ray_cam->im->w) {
			image_i = 0;
			image_j++;
		}    
	}

	// write rendered image to file when done

	else if (!wrote_image) {

		write_PPM("output.ppm", ray_cam->im);
		// write depth image
		double multiplier = (0.0f - 1.0f) / (dptMax - dptMin);
		double addend = 1.0f - dptMin * multiplier;
		for (int i = 0; i < ray_cam->im->w * ray_cam->im->h; i++)
		{
			if (dptImage[i] != MAXDEPTH)
			{
				// normailze to 0 - 1.0
				dptImage[i] = dptImage[i] * multiplier + addend;
			}

			else
				dptImage[i] = 0.0f;

		} 

		
		write_DPT("depthInfo.bin", dptImage, ray_cam->im->w, ray_cam->im->h);

		wrote_image = true;
	}

	glutPostRedisplay();
}

//----------------------------------------------------------------------------

// show the image so far

void display(void)
{
	// draw it!

	glPixelZoom(1, -1);
	glRasterPos2i(0, ray_cam->im->h);

	// display color image
	glDrawPixels(ray_cam->im->w, ray_cam->im->h, GL_RGBA, GL_FLOAT, ray_cam->im->data);
	
	// display depth image
	//glDrawPixels(ray_cam->im->w, ray_cam->im->h, GL_LUMINANCE, GL_FLOAT, dptImage);

	glFlush ();
}

//----------------------------------------------------------------------------

void init()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, ray_cam->im->w, 0.0, ray_cam->im->h);
}

//----------------------------------------------------------------------------

int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	// initialize scene (must be done before scene file is parsed)

	init_raytracing();

	if (argc == 2)
		parse_scene_file(argv[1], ray_cam);
	else {
		printf("missing .scene file\n");
		exit(1);
	}

	// opengl business

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(ray_cam->im->w, ray_cam->im->h);

	// init depth image
	dptImage = (GLfloat*)calloc(ray_cam->im->w * ray_cam->im->h, sizeof(GLfloat));

	glutInitWindowPosition(500, 300);
	glutCreateWindow("hw3");
	init();

	glutDisplayFunc(display); 
	glutIdleFunc(idle);

	glutMainLoop();

	return 0; 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define USE_MKL

#ifdef USE_MKL
// _mm_macc...
#else
// 
#endif // USE_MKL
