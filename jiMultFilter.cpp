#include <ai.h>
#define SIXTH 0.1666666667f

//HSV to RGB functions from SItoA
AtRGB HSVtoRGB(AtRGB& hsv)
{
   // h is 0->1, s 0->1  v 0->1
   float f, p, q, t;
   int i;

   AtRGB rgb = AI_RGB_ZERO;

   if(hsv[0] == -1.0f || hsv[1] == 0.0f)
   {
      // achromatic
      rgb[0] = hsv[2];
      rgb[1] = hsv[2];
      rgb[2] = hsv[2];
   }
   else
   {
      // chromatic
      float hue = hsv[0];
      hue = (hue - std::floor(hue));
      hue /= SIXTH;


      i = (int) floor(hue);
      f = hue - i;
      p = hsv[2] * (1.0f - (hsv[1]));
      q = hsv[2] * (1.0f - (hsv[1] * f));
      t = hsv[2] * (1.0f - (hsv[1] * (1.0f - f)));

      switch(i)
      {
         case 0: rgb[0] = hsv[2]; rgb[1] = t; rgb[2] = p; break;
         case 1: rgb[0] = q; rgb[1] = hsv[2]; rgb[2] = p; break;
         case 2: rgb[0] = p; rgb[1] = hsv[2]; rgb[2] = t; break;
         case 3: rgb[0] = p; rgb[1] = q; rgb[2] = hsv[2]; break;
         case 4: rgb[0] = t; rgb[1] = p; rgb[2] = hsv[2]; break;
         case 5: rgb[0] = hsv[2]; rgb[1] = p; rgb[2] = q; break;
      }
   }

   //rgb.a = hsv.a; // straight copy for alpha

   return rgb;
}

AtRGB RGBtoHSV(AtRGB& RGB)
{
   // h is 0->1, s 0->1  v 0->1

   AtRGB hsv = AI_RGB_ZERO;

   float maxcolor = AiMax(RGB[0], AiMax(RGB[1], RGB[2])); // Luminant delta
   float mincolor = AiMin(RGB[0], AiMin(RGB[1], RGB[2]));
   float delta = maxcolor - mincolor;

   hsv[2] = maxcolor; // Value

   if( maxcolor == mincolor || maxcolor == 0.0f)
   {
      // Achromatic
      hsv[0] = -1.0f;
      hsv[1] = 0.0f;
   }
   else
   {
      /* chromatic hue, saturation */

      hsv[1] = delta / maxcolor;	

      if (RGB[0] == maxcolor)
         hsv[0] = (RGB[1] - RGB[2]) / delta; // between yellow and magenta
      else if (RGB[1] == maxcolor)
         hsv[0] = 2.0f + ((RGB[2] - RGB[0]) / delta); // between cyan and yellow
      else
         hsv[0] = 4.0f + ((RGB[0] - RGB[1]) / delta); // between magenta and cyan

      hsv[0] *= SIXTH;

      while (hsv[0] < 0.0f)
         hsv[0] += 1.0f; // make sure it is nonnegative 
   }

   //hsv.a = RGB.a; // straight copy for alpha

   return hsv;
}
 
 
AI_SHADER_NODE_EXPORT_METHODS(JIMultFilterMtd);
 
 
enum jiMultFilterParams {
    //p_objects,
    p_user_data_name,
    p_user_data_default,
    p_mask,
    p_intensity,
    p_exposure,
    p_multiply,
    p_HSV,
};
 
 
    node_parameters
{
    //AiParameterStr("objects", "");
    AiParameterStr("user_data_name", "");
    AiParameterFlt("user_data_default", 0);
    AiParameterFlt("mask", 1.0f);
    AiParameterFlt("intensity", 1.0f);
    AiParameterFlt("exposure", 0.0f);
    AiParameterRGB("multiply", 1.0f, 1.0f, 1.0f);
    AiParameterVec("HSV", 0.0f, 1.0f, 1.0f);
}
 
namespace {
typedef struct 
{
   AtString user_data_name;
   float user_data_default;
   float mask;
   float intensity;
   float exposure;
   AtRGB multiply;
   AtVector HSV;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->user_data_name = AiNodeGetStr(node, "user_data_name");
   data->user_data_default = AiNodeGetFlt(node, "user_data_default");
   data->mask = AiNodeGetFlt(node, "mask");
   data->intensity = AiNodeGetFlt(node, "intensity");
   data->exposure = AiNodeGetFlt(node, "exposure");
   data->multiply = AiNodeGetRGB(node, "multiply");
   data->HSV = AiNodeGetVec(node, "HSV");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}
 
shader_evaluate
{
   // test if we are running as a light filter
   if (sg->light_filter)
   {
      //read user data (if nothing is provided everything will be affected)
      float user_data_default = AiShaderEvalParamFlt(p_user_data_default);
      const AtString user_data_name = AiShaderEvalParamStr(p_user_data_name);
      if ( user_data_name == AtString("") )
         user_data_default = 1; 
      
      AiUDataGetFlt( user_data_name, user_data_default );

      float mask = AiClamp( AiShaderEvalParamFlt(p_mask), 0.0f, 1.0f ) * user_data_default;
      float intensity = AiClamp( AiShaderEvalParamFlt(p_intensity), 0.0f, 9999.0f );
      float exposure = AiShaderEvalParamFlt(p_exposure);
      AtRGB multiply = AiShaderEvalParamRGB(p_multiply);
      AtVector HSV = AiShaderEvalParamVec(p_HSV);

      //combine RGB and HSV
      AtRGB lgt = sg->light_filter->Liu;
      AtRGB lgt_modified = lgt * intensity * pow(2,exposure) * multiply;
      AtRGB lgt_to_HSV = RGBtoHSV( lgt_modified );
      lgt_to_HSV[0] = lgt_to_HSV[0] + HSV[0];
      lgt_to_HSV[1] = lgt_to_HSV[1] * HSV[1];
      lgt_to_HSV[1] = AiClamp(lgt_to_HSV[1], 0.0f, 1.0f);
      lgt_to_HSV[2] = lgt_to_HSV[2] * HSV[2];

      sg->light_filter->Liu = AiLerp( mask, lgt, HSVtoRGB(lgt_to_HSV) );
   }
}

node_loader
{
   if (i > 0)
      return false;
   node->methods        = JIMultFilterMtd;
   node->output_type    = AI_TYPE_NONE;
   node->name           = "jiMultFilter";
   node->node_type  = AI_NODE_SHADER;
   strcpy(node->version, AI_VERSION);
   return true;
}
