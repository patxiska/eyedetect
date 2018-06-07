/**
 * Copyright (C) Marcos Rogani <marcos.rogani@gmail.com>
 * 
 */ 
 

#ifndef EYEDETECTOR_H_
#define EYEDETECTOR_H_

#include <opencv/cv.h>

#include <list>

#ifdef _EiC
#define WIN32
#endif

#define k_a 1
#define k_r 0
#define k_d 0

using namespace std;

CvSeq* filter(IplImage* img, CvSeq* contours, list<int(*)(IplImage*,CvSeq*)> filtros)
{
	CvSeq *ptr;
	ptr = contours;
	int continuar;
	while (ptr != NULL){
		continuar =1;
		for(list<int(*)(IplImage*,CvSeq*)>::iterator it=filtros.begin(); it != filtros.end() && continuar;it++)
			continuar = ((int(*)(IplImage*,CvSeq*) )(*it))(img,ptr);
		if (!continuar){
			if (ptr == contours){						// si elimino el primer elemento de la lista
				if(ptr->h_next){						// si la la longitud de la lista es > 1
					contours = ptr->h_next;
					ptr = contours;
					ptr->h_prev = NULL;
				}else{
					ptr = contours = NULL;
				}
			}
			else
			{											// si elimino en otra parte de la lista
				ptr->h_prev->h_next = ptr->h_next;
				if (ptr->h_next != NULL)
					ptr->h_next->h_prev = ptr->h_prev;
				ptr = ptr->h_next;
			}
		}else
		{
			ptr = ptr->h_next;
		}
	}
	return contours;
}

double distancia_idx(IplImage* img, CvSeq *contour1, CvSeq *contour2){
	CvRect box1 = cvBoundingRect(contour1,0);
	CvRect box2 = cvBoundingRect(contour2,0);
	double dist = 1-abs(6*box1.width- sqrt(pow((box1.x+box1.width/2)-(box2.x+box2.width/2),2)+pow((box1.y+box1.height/2)-(box2.y+box2.height/2),2)));
	return dist;
}

double ratio_idx(IplImage* img, CvSeq *contour1, CvSeq *contour2){
	CvRect box1 = cvBoundingRect(contour1,0);
	CvRect box2 = cvBoundingRect(contour2,0);
	return 1-sqrt(pow((box1.width-box2.width),2)+pow((box1.height-box2.height),2));
}

double sum(IplImage* img, CvSeq *contour){

	CvRect box = cvBoundingRect(contour,0);
	double sum = 0;
	double acum = 0;
	float* ptr;
	for( int y=box.y+1; y<box.y+box.height-1; y++ ) {
		ptr = (float*) (img->imageData + y * img->widthStep);
		for( int x=box.x+1; x<box.x+box.width-1; x++ ){
			if (ptr[x] != 0)
				acum +=1; 
			sum += ptr[x];
		}
	}
	return sum/acum;
}

double area_idx(IplImage* img, CvSeq *contour1, CvSeq *contour2){
	return sum(img, contour1)+sum(img, contour2);
}


#define apertura 30
int angulo_ok(CvSeq* c1,CvSeq* c2){
	CvRect box1 = cvBoundingRect(c1,0);
	CvRect box2 = cvBoundingRect(c2,0);
	
	if(box1.x == box2.x) return 0;
		double op = fabs((box1.y+box1.height/2)-(box2.y+box2.height/2));
		double ad = fabs((box1.x+box1.width/2)-(box2.x+box2.width/2));
		double alfa = atan(op/ad)*180/M_PI;
		
		if((alfa > apertura && alfa < (180-apertura))|| (alfa > (180+apertura) && alfa < (360-apertura)))
			return 0;

		return 1;
}

CvSeq* dosOjos_mejor(IplImage* img, CvSeq *contourList){
	CvSeq *ptr,*ptr2,*ptrF,*ptrF2;
	if (!contourList)								// Lista vacia 
		return contourList;
	if(contourList->h_next != NULL){
		if (contourList->h_next->h_next == NULL)
			if (angulo_ok(contourList,contourList->h_next))
				return contourList;						// 2 elementos
			else return NULL;
	} else{
		return contourList;							// 1 elemento

	}

	ptr = contourList;
	ptr2 = ptr->h_next;

	ptrF = ptr;
	ptrF2 = ptr2;
	double aux, sum = k_r*ratio_idx(img,ptr,ptr2)+k_a*area_idx(img,ptr,ptr2)+k_d*distancia_idx(img,ptr,ptr2);  

	while (ptr != NULL){
		ptr2 = ptr->h_next;
		while(ptr2 != NULL){
			aux = k_r*ratio_idx(img,ptr,ptr2)+k_a*area_idx(img,ptr,ptr2)+k_d*distancia_idx(img,ptr,ptr2);  
			if (aux > sum && angulo_ok(ptr,ptr2)){
				ptrF = ptr;
				ptrF2 = ptr2;
				sum = aux;
			}
			ptr2 = ptr2->h_next;
		}
		ptr = ptr->h_next;
	}

	// limpio la lista dejando solo a los 2 "mejores"
	ptr = contourList;
	while (ptr != NULL){
		if (ptr != ptrF && ptr != ptrF2){
			// elimino el elemento *ptrMin
			if (ptr == contourList)
			{						// si elimino el primer elemento de la lista
				if(ptr->h_next){						// si la la longitud de la lista es > 1
					contourList = ptr->h_next;
					contourList->h_prev = NULL;
				}else{
					ptr = contourList = NULL;
				}
			}
			else				
			{											// si elimino en otra parte de la lista
				ptr->h_prev->h_next = ptr->h_next;
				if (ptr->h_next != NULL)
					ptr->h_next->h_prev = ptr->h_prev;
			}
		}
		if(ptr)
			ptr = ptr->h_next;
	}

	if (angulo_ok(contourList,contourList->h_next))
		return contourList;						// 2 elementos
	else return NULL;
}

// filtra por la relacion de aspecto
int ratio_filter(IplImage* img, CvSeq *contour)
{
	CvRect box = cvBoundingRect(contour,0);
	double R = 1.0*box.width / box.height;
	if (R > 0.9 && R < 4) 
		return 1;
	return 0;
} 

// filtra por proximidad al borde de la imagen
int noBorder_filter(IplImage* img, CvSeq *contour)
{
	CvRect box = cvBoundingRect(contour,0);
	if (box.x <= 1 || box.y <= 1 || (box.x+box.width)+1 >= img->width || (box.y+box.height+1 >= img->height)) 
		return 0;
	return 1;
} 

// filtra por porcion ocupada dentro del cuadrado que lo contiene
int solidity_filter(IplImage* img, CvSeq *contour)
{
	CvRect box = cvBoundingRect(contour,0);
	uchar* ptr;
	unsigned int sum = 0;
	for( int y=box.y+1; y<box.y+box.height-1; y++ ) {
		ptr = (uchar*) (img->imageData + y * img->widthStep);
		for( int x=box.x+1; x<box.x+box.width-1; x++ )
			sum += ptr[x];
	}

	if(sum == 0)
		return 0;
	double S = sum/(1.0*box.width*box.height);
	if (S > 0.7) 
		return 1;
	return 0;
} 


CvSeq* eyedetector( IplImage* image_in, CvMemStorage* storage) // requiere imagen normalizada con un maximo de 200x200 pix
{

	int y,x;
	double minVal,maxVal;
	IplImage* image = image_in;
	IplImage* image_bgr = NULL;
	IplImage* ycrcb = cvCreateImage( cvSize(image->width,image->height), 8, 3 );
	IplImage* yC = cvCreateImage( cvSize(image->width,image->height), 8, 1 );
	IplImage* cb = cvCreateImage( cvSize(image->width,image->height), 32, 1 );
	IplImage* cr = cvCreateImage( cvSize(image->width,image->height), 32, 1 );
	IplImage* eyeMapC = cvCreateImage( cvSize(image->width,image->height), 8, 1 );
	IplImage* eyeMapL = cvCreateImage( cvSize(image->width,image->height), 32, 1 );
	IplImage* eyeMapRes = cvCreateImage( cvSize(image->width,image->height), 32, 1 );
	IplImage* yDilate = cvCreateImage( cvSize(image->width,image->height), 8, 1 );
	IplImage* yErode = cvCreateImage( cvSize(image->width,image->height), 8, 1 );
	IplImage* eyeMapBinaria = cvCreateImage( cvSize(eyeMapRes->width,eyeMapRes->height), 8, 1 );

	if( image )
	{

		if(image->nChannels == 1){
			image_bgr = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U , 3);
			cvCvtColor(image ,image_bgr, CV_GRAY2BGR);
			image = image_bgr;
		}



		IplConvKernel* se = cvCreateStructuringElementEx(8,8,4,4, 0, NULL);
		se = NULL;
		uchar* ptr,*ptreyeBinMap,*ptryErode,*ptryDilate, *ptreyeMapC;
		float *ptreyeMapL,*ptreyeMap,*ptrcb,*ptrcr;

		// cambio de espacio BGR -> YCrcb
		cvCvtColor(image, ycrcb, CV_BGR2YCrCb);

		cvSplit(ycrcb,yC,NULL,NULL,NULL);

		// calculo cb² y cr²
		for( y=0; y<ycrcb->height; y++ )
		{
			ptr = (uchar*) (ycrcb->imageData + y * ycrcb->widthStep);
			ptrcb = (float*) (cb->imageData + y * cb->widthStep);
			ptrcr = (float*) (cr->imageData + y * cr->widthStep);
			for(  x=0; x<ycrcb->width; x++ ) 
			{
				ptrcr[x] = (float) pow(255-ptr[3*x+1],2);
				ptrcb[x] = (float) pow(ptr[3*x+2],2);
			}
		}

		// normalizo el rango de cr² y cb²
		cvMinMaxLoc( cr, &minVal,&maxVal, NULL, NULL, NULL);

		for( y=0; y<cr->height; y++ ){
			ptrcr = (float*) (cr->imageData + y * cr->widthStep);
			for(  x=0; x<cr->width; x++ ) {
				ptrcr[x] = (float) 255*(ptrcr[x]-minVal)/(maxVal-minVal);
			}
		}

		cvMinMaxLoc( cb, &minVal,&maxVal, NULL, NULL, NULL);

		for( y=0; y<cb->height; y++ ){
			ptrcb = (float*) (cb->imageData + y * cb->widthStep);
			for(  x=0; x<cb->width; x++ ) {
				ptrcb[x] = (float) 255*(ptrcb[x]-minVal)/(maxVal-minVal);
			}
		}

		// calculo el eyeMap de la crominancia
		for(  y=0; y<eyeMapC->height; y++ )
		{
			ptreyeMapC = (uchar*) (eyeMapC->imageData + y * eyeMapC->widthStep);
			ptrcb = (float*) (cb->imageData + y * cb->widthStep);
			ptrcr = (float*) (cr->imageData + y * cr->widthStep);
			for( x=0; x<eyeMapC->width; x++ )
				ptreyeMapC[x] = round((ptrcb[x]+ptrcr[x]+ ptrcb[x]/(1+ptrcr[x]))/3);			//cvround mas rapido?
		}

		cvEqualizeHist( eyeMapC, eyeMapC );

		cvErode(yC,yErode, se, 1);	
		cvDilate(yC,yDilate, se, 1);

		// calculo el eyemap de luminancia
		for(  y=0; y<eyeMapL->height; y++ ) {
			ptreyeMapL = (float*) (eyeMapL->imageData + y * eyeMapL->widthStep);
			ptryErode = (uchar*) (yErode->imageData + y * yErode->widthStep);
			ptryDilate = (uchar*) (yDilate->imageData + y * yDilate->widthStep);
			for(  x=0; x<eyeMapL->width; x++ )
				ptreyeMapL[x] = (float) ptryDilate[x]/(1+ptryErode[x]);

		}

		IplImage* eyeMap = cvCloneImage(eyeMapL);

		// combino ambos mapas
		for( y=0; y<eyeMap->height; y++ ) {
			ptreyeMap = (float*) (eyeMap->imageData + y * eyeMap->widthStep);
			ptreyeMapC = (uchar*) (eyeMapC->imageData + y * eyeMapC->widthStep);
			for( x=0; x<eyeMap->width; x++ ){
				ptreyeMap[x] = (float) ptreyeMap[x]*ptreyeMapC[x];
			}
		}

		cvDilate(eyeMap,eyeMapRes,se,1);
		
		int th = 120;
		cvMinMaxLoc( eyeMapRes, &minVal, &maxVal, NULL,NULL, 0 );

		// normaliza y binariza la imagen, (solo normaliza la parte de interes)
		for( y=0; y<eyeMapRes->height; y++ ) {
			ptreyeMap = (float*) (eyeMapRes->imageData + y * eyeMapRes->widthStep);
			ptreyeBinMap = (uchar*) (eyeMapBinaria->imageData + y * eyeMapBinaria->widthStep);
			for( x=0; x<eyeMapRes->width; x++ ){
				if( x > eyeMapRes->width*0.15 && x<(eyeMapRes->width-eyeMapRes->width*0.15) && y < eyeMapRes->height*0.4 && y > eyeMapRes->height*0.2)
					ptreyeBinMap[x] = (255*(ptreyeMap[x]-minVal)/(maxVal-minVal) > th) ? 1 : 0;
				else
					ptreyeBinMap[x] = 0;
			}
		}

		cvDilate(eyeMapBinaria,yC,se,1);
		cvDilate(yC,eyeMapBinaria,se,1);

		CvSeq* contour = 0;

		cvClearMemStorage(storage);
		cvFindContours( eyeMapBinaria, storage, &contour, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0) );

		list<int(*)(IplImage*,CvSeq*)> filtros;
		filtros.push_back(&noBorder_filter);
		filtros.push_back(&ratio_filter);
		filtros.push_back(&solidity_filter);

		// filtra los contornos
		contour = filter(eyeMapBinaria,contour,filtros);

		// obtiene dos ojos
		contour = dosOjos_mejor(eyeMapRes,contour);


		/***** Aplica el procedimiento sucesivamente bajando el threshold *****/

			while((contour==NULL || contour->h_next == NULL)&& th>0){
			th -= 5;
			for( y=0; y<eyeMapRes->height; y++ ) {
				ptreyeMap = (float*) (eyeMapRes->imageData + y * eyeMapRes->widthStep);
				ptreyeBinMap = (uchar*) (eyeMapBinaria->imageData + y * eyeMapBinaria->widthStep);
				for( x=0; x<eyeMapRes->width; x++ ){
					if( x > eyeMapRes->width*0.15 && x<(eyeMapRes->width-eyeMapRes->width*0.15) && y < eyeMapRes->height*0.4 && y > eyeMapRes->height*0.2)
						ptreyeBinMap[x] = (255*(ptreyeMap[x]-minVal)/(maxVal-minVal) > th) ? 1 : 0;
					else
						ptreyeBinMap[x] = 0;
				}
			}

			cvDilate(eyeMapBinaria,yC,se,1);
			cvDilate(yC,eyeMapBinaria,se,1);

			cvClearMemStorage(storage);
			contour = 0;

			cvFindContours( eyeMapBinaria, storage, &contour, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0) );

			contour = filter(eyeMapBinaria,contour,filtros);

			contour = dosOjos_mejor(eyeMapRes,contour);

		}

		/*********************************************************************/

		if(image_bgr)
			cvReleaseImage(&image_bgr);
		cvReleaseImage(&ycrcb);
		cvReleaseImage(&eyeMapC);
		cvReleaseImage(&eyeMapL);
		cvReleaseImage(&yDilate);
		cvReleaseImage(&yErode);
		cvReleaseImage(&eyeMapBinaria);
		cvReleaseImage(&eyeMapRes);
		cvReleaseImage(&yC);
		cvReleaseImage(&cb);
		cvReleaseImage(&cr);
		cvReleaseImage(&eyeMap);
		cvReleaseStructuringElement(&se);
		return contour;
	}

	return NULL;
}

#endif /*EYEDETECTOR_H_*/
