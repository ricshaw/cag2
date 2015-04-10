#include <stdio.h>
#include <stdlib.h>		/* for abs */
#include <float.h>		/* for FLT_MAX */
#include <math.h>
#include <string.h>
#include <assert.h>
#include "shader.h"

/******************************************************************************
 * MAYA LIGHT LINKING
 *****************************************************************************/
#include "mayaapi.h"

struct maya_illum_lambert {
	miColor     ambience;   /* ambient color multiplier */
	miColor     ambient;    /* ambient color */
	miColor     diffuse;    /* diffuse color */
	int         mode;       /* light mode: 0..2, 4 */
	int         i_light;    /* index of first light */
	int         n_light;    /* number of lights */
	miTag       light[1];   /* list of lights */
	/*********************************************************************
	 * MAYA LIGHT LINKING
	 *********************************************************************/
	// No longer necessary as of Maya 2008, as long as light link mode
	// is set to 4.
	//
	//miTag       lightLink;  /* light linker */
	//miBoolean   miLightLink;/* enable auto light links on light array? */
};


extern "C" DLLEXPORT int maya_illum_lambert_version(void) {return(4);}

extern "C" DLLEXPORT miBoolean maya_illum_lambert(
	miColor		*result,
	miState		*state,
	struct maya_illum_lambert *paras)
{
	miColor		*ambi, *diff;
	miTag		*light;		/* tag of light instance */
	int		n_l;		/* number of light sources */
	int		i_l;		/* offset of light sources */
	int		m;		/* light mode: 0=all, 1=incl, 2=excl, 4=native mental ray */
	int		samples;	/* # of samples taken */
	miColor		color;		/* color from light source */
	miColor		sum;		/* summed sample colors */
	miScalar	dot_nl;		/* dot prod of normal and dir*/

	/*********************************************************************
	 * MAYA LIGHT LINKING 
	 *********************************************************************/
	// No longer necessary as of Maya 2008, as long as light link mode
	// is set to 4.
	//
	//int numLightLinks;
	//miTag lightLink, *lightLinks;

	/* check for illegal calls */
	if (state->type == miRAY_SHADOW || state->type == miRAY_DISPLACE ) {
		return(miFALSE);
	}

	ambi =  mi_eval_color(&paras->ambient);
	diff =  mi_eval_color(&paras->diffuse);
	m    = *mi_eval_integer(&paras->mode);

	*result    = *mi_eval_color(&paras->ambience);	/* ambient term */
	result->r *= ambi->r;
	result->g *= ambi->g;
	result->b *= ambi->b;

	n_l   = *mi_eval_integer(&paras->n_light);
	i_l   = *mi_eval_integer(&paras->i_light);
	light =  mi_eval_tag(paras->light) + i_l;

	if (m == 1)		/* modify light list (inclusive mode) */
		mi_inclusive_lightlist(&n_l, &light, state);
	else if (m == 2)	/* modify light list (exclusive mode) */
		mi_exclusive_lightlist(&n_l, &light, state);
	else if (m == 4) {
		n_l = 0;
		light = 0;
	}

	/*********************************************************************
	 * MAYA LIGHT LINKING
	 * Get light linker shader.
	 *********************************************************************/
	// No longer necessary as of Maya 2008, as long as light link mode
	// is set to 4.
	//
	//lightLink = *mi_eval_tag(&paras->lightLink);
	//if (lightLink) {
	//	mayabase_lightlink_get(
	//		lightLink, &numLightLinks, &lightLinks, state);
	//}

	/* Loop over all light sources */
	if (m==4 || n_l)
	for (mi::shader::LightIterator iter(state, light, n_l);
	     !iter.at_end(); ++iter) {
		sum.r = sum.g = sum.b = 0;

		/*************************************************************
		 * MAYA LIGHT LINKING
		 * Check linking of current light.
		 *************************************************************/
		// No longer necessary as of Maya 2008, as long as light link mode
		// is set to 4.
		//
		//if (lightLink && !mayabase_lightlink_check(
		//	*iter, numLightLinks, lightLinks, state)) {
		//	/* Light not linked to current instance. */
		//	continue;
		//}

		while (iter->sample()) {
			dot_nl = iter->get_dot_nl();
			iter->get_contribution(&color);
			sum.r += dot_nl * diff->r * color.r;
			sum.g += dot_nl * diff->g * color.g;
			sum.b += dot_nl * diff->b * color.b;
		}
		samples = iter->get_number_of_samples();
		if (samples > 0) {
			result->r += sum.r / samples;
			result->g += sum.g / samples;
			result->b += sum.b / samples;
		}
	}

	/* add contribution from indirect illumination (caustics) */
	mi_compute_irradiance(&color, state);
	result->r += color.r * diff->r;
	result->g += color.g * diff->g;
	result->b += color.b * diff->b;
	result->a  = 1;
	return(miTRUE);
}
