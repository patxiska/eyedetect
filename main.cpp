/**
 * Copyright (C) Marcos Rogani <marcos.rogani@gmail.com>
 * 
 * 
 */ 

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <stdio.h>

#include "eyedetector.h"

#ifdef _EiC
#define WIN32
#endif

static CvMemStorage* storage = 0;
static CvMemStorage* storage2 = 0;
static CvHaarClassifierCascade* cascade = 0;


void detect_and_draw( IplImage* img, double scale_factor, int min_neighbors, int noeye, int flags, CvSize min_size );
IplImage* CopySubImage(IplImage* image, int xorigen, int yorigen, int width, int height);

const char* cascade_name;

int main( int argc, char** argv )
{
	CvCapture* capture = 0;
	IplImage *frame, *frame_copy = 0;
	const char* input_name = "0";
	int i;
	double scale_factor = 1.2;
	int min_neighbors = 3;
	int noeye = 0;
	int flags = 0;/*CV_HAAR_DO_CANNY_PRUNING*/
	CvSize min_size = cvSize(48,48);

	if( argc == 1 )
	{
		fprintf( stderr, "ERROR: Could not load classifier cascade\n" );
		fprintf( stderr,
				"Usage: realTimeEyeDetect  "
				"  [ -c <cascade_xml_path> ]\n"
				"  [ -noeye (no deteccion de ojos)]"
				"  [ -sf < scale_factor = %f > ]\n"
				"  [ -mn < min_neighbors = %d > ]\n"
				"  [ -fl < flags = %d > ]\n"
				"  [ -ms < min_size = %d %d > ]\n"
				"  [ filename | camera_index = %s ]\n",
				scale_factor, min_neighbors, flags, min_size.width, min_size.height, input_name );
		fprintf( stderr, "See also: cvHaarDetectObjects() about option parameters.\n" );
		return -1;
	}

	for( i = 1; i < argc; i++ )
	{
		if( !strcmp( argv[i], "-c" ) )
		{
			cascade_name = argv[++i];
		}
		else if( !strcmp( argv[i], "-sf" ) )
		{
			scale_factor = (float) atof( argv[++i] );
		}
		else if( !strcmp( argv[i], "-noeye" ) )
		{
			noeye = 1;
		}
		else if( !strcmp( argv[i], "-mn" ) )
		{
			min_neighbors = atoi( argv[++i] );
		}
		else if( !strcmp( argv[i], "-fl" ) )
		{
			flags = CV_HAAR_DO_CANNY_PRUNING;
		}
		else if( !strcmp( argv[i], "-ms" ) )
		{
			min_size = cvSize( atoi( argv[++i] ), atoi( argv[++i] ) );
		}
		else
		{
			input_name = argv[i];
		}
	}

	cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
	storage = cvCreateMemStorage(0);
	storage2 = cvCreateMemStorage(0);

	if( !input_name || (isdigit(input_name[0]) && input_name[1] == '\0') )
		capture = cvCaptureFromCAM( !input_name ? 0 : input_name[0] - '0' );
	else
		capture = cvCaptureFromAVI( input_name );

	cvNamedWindow( "result", 1 );

	if( capture )
	{
		for(;;)
		{
			if( !cvGrabFrame( capture ))
				break;
			frame = cvRetrieveFrame( capture );
			if( !frame )
				break;
			if( !frame_copy )
				frame_copy = cvCreateImage( cvSize(frame->width,frame->height),
						IPL_DEPTH_8U, frame->nChannels );
			if( frame->origin == IPL_ORIGIN_TL )
				cvCopy( frame, frame_copy, 0 );
			else
				cvFlip( frame, frame_copy, 0 );

			detect_and_draw( frame_copy, scale_factor, min_neighbors, noeye, flags, min_size );
	//		cvWaitKey(0);
			if( cvWaitKey( 10 ) >= 0 )
				break;
		}

		cvReleaseImage( &frame_copy );
		cvReleaseCapture( &capture );
	}
	else
	{
		const char* filename = input_name ? input_name : (char*)"lena.jpg";
		IplImage* image = cvLoadImage( filename, 1 );

		if( image )
		{
			detect_and_draw( image, scale_factor, min_neighbors, noeye, flags, min_size );
			cvWaitKey(0);
			cvReleaseImage( &image );
		}
		else
		{
			/* assume it is a text file containing the
               list of the image filenames to be processed - one per line */
			FILE* f = fopen( filename, "rt" );
			if( f )
			{
				char buf[1000+1];
				while( fgets( buf, 1000, f ) )
				{
					int len = (int)strlen(buf);
					while( len > 0 && isspace(buf[len-1]) )
						len--;
					buf[len] = '\0';
					image = cvLoadImage( buf, 1 );
					if( image )
					{
						detect_and_draw( image, scale_factor, min_neighbors, noeye, flags, min_size );
						cvWaitKey(0);
						cvReleaseImage( &image );
					}
				}
				fclose(f);
			}
		}

	}

	cvDestroyWindow("result");

	return 0;
}

void detect_and_draw( IplImage* img, double scale_factor, int min_neighbors, int noeye, int flags, CvSize min_size )
{
	static CvScalar colors[] =
	{
			{0,0,255},
			{0,128,255},
			{0,255,255},
			{0,255,0},
			{255,128,0},
			{255,255,0},
			{255,0,0},
			{255,0,255}
	};

	double scale = 1.0;
	double factor_scala =0.0;
	IplImage* crop_img, *gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
	IplImage* norm_img = cvCreateImage( cvSize(200,200), 8, 3 );
	IplImage* small_img = cvCreateImage( cvSize( cvRound (img->width/scale), cvRound (img->height/scale)), 8, 1 );

	int i;

	cvCvtColor( img, gray, CV_BGR2GRAY );
	cvResize( gray, small_img, CV_INTER_LINEAR );
	cvEqualizeHist( small_img, small_img );
	cvClearMemStorage( storage );

	if( cascade )
	{
		double t = (double)cvGetTickCount();
		CvRect *r_face;
		CvSeq* ptr;
		CvSeq* eyes;
		CvPoint center;
		int radius;

		CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage, scale_factor, min_neighbors, flags, min_size );

		t = (double)cvGetTickCount() - t;
		printf( "FACE detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );

		for( i = 0; i < (faces ? faces->total : 0); i++ )
		{

			r_face = (CvRect*) cvGetSeqElem( faces, i );
			crop_img = CopySubImage(img, r_face->x, r_face->y, r_face->width, r_face->height);

			cvClearMemStorage( storage2 );

			center.x = cvRound((r_face->x + r_face->width*0.5)*scale);
			center.y = cvRound((r_face->y + r_face->height*0.5)*scale);
			radius = cvRound((r_face->width + r_face->height)*0.25*scale);
			cvCircle( img, center, radius, colors[i%8], 3, 8, 0 );

			t = (double)cvGetTickCount();
			if(!noeye){
				if(crop_img->width > 200){ // tamaño mayor a 200, normalizo
					cvResize(crop_img, norm_img);
					factor_scala = (crop_img->width-norm_img->width)/(1.0*(crop_img->width));
					eyes = eyedetector( norm_img, storage2);
				}
				else{	// tamaño menor a 200x200
					factor_scala=0.0;
					eyes = eyedetector( crop_img, storage2);
				}
			}
			t = (double)cvGetTickCount() - t;
			printf( "EYE detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );

			if(noeye) continue;
			if(!eyes) continue;
			int detcount,k, total = 0;

			for(ptr = eyes;ptr != NULL; ptr = ptr->h_next) total++;
			detcount = total;
			printf("ojos: %d\n",total);
			for( k = 0; k < detcount; k++ )
			{
				CvRect r_eye = cvBoundingRect( eyes,0);
				cvRectangle( img, cvPoint( r_eye.x+round(r_eye.x*factor_scala)+r_face->x, r_eye.y+round(r_eye.y*factor_scala)+r_face->y ), cvPoint( r_eye.x+round(r_eye.x*factor_scala)+r_face->x + r_eye.width+round(r_eye.width*factor_scala), r_eye.y+round(r_eye.y*factor_scala)+r_face->y + r_eye.height+ round(r_eye.height*factor_scala)), CV_RGB( 255, 0, 0 ), 1 );
				eyes = eyes->h_next;
			}

			cvShowImage( "result", img );
			cvReleaseImage( &crop_img );
		}
	}

	cvShowImage( "result", img );

	cvReleaseImage( &gray );
	cvReleaseImage( &norm_img );
	cvReleaseImage( &small_img );
}

IplImage* CopySubImage(IplImage* image, int xorigen, int yorigen, int width, int height) {
	CvRect roi;
	IplImage *res;

	roi.x = xorigen;
	roi.y = yorigen;
	roi.width = width;
	roi.height = height;

	if(xorigen+width > image->width || yorigen+height > image->height || xorigen <0 || yorigen <0)
		return NULL;
	cvSetImageROI(image, roi);
	res = cvCreateImage(cvSize(roi.width,roi.height), image->depth, image->nChannels);
	cvCopy(image, res, NULL);
	cvResetImageROI(image);
	return res;
}

