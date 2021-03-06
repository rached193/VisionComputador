/*
 * Autores: Aron Collados (626558)
 *          Cristian Roman (646564)
 */
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdio.h>

using namespace cv;
using namespace std;

int scale = 1;
int delta = 0;
int ddepth = CV_32F;
int UMBRAL = 50;
float MARGEN=0.3;


void getLines(vector<Vec2f> in,Mat grad,Mat dir,float diff, int r){
	float angA=(CV_PI/r);
	int maxX=max(grad.rows,grad.cols);
        int values[r][r];  //tabla de votos
	for(int i=0;i<r;i++){
        	for(int o=0;o<r;o++){
    			values[i][o]=0;
		}
    	}
	for(int i=0;i<grad.cols;i++){
    	    for(int o=0;o<grad.rows;o++){
    		int xP=i;		 //Posicion inicial en el eje X
		int yP=o;            //Posicion inicial en el eje Y

    		if((int)grad.at<uchar>(yP,xP)>UMBRAL){
    			//cout<<xP<<" - "<<yP<<" : "<<(int)grad.at<uchar>(yP,xP)<<endl;
    			float angle=dir.at<float>(yP,xP);
			//cout<<xP<<" - "<<yP<<" : "<<angle<<endl;
			for(float ang=angle-diff;ang<angle+diff;ang+=angA){
				float xD=sin(ang);   //Modulo de la direcion en el eje X
				float yD=cos(ang);   //Modulo de la direcion en el eje Y
				//xP*yD+yP*yD-p=0
				float p=xP*xD+yP+yD;
				//cout<<ang<<"/"<<CV_PI*2<<" -! "<<p++maxX/2<<"/"<<maxX<<endl;
				int intp=round((p+maxX/2)*r/(maxX));
				int inta=round((ang)*r/(CV_PI*2	));
				cout<<intp<<"/"<<r<<" -> "<<inta<<"/"<<r<<" : "<<values[intp][inta]<<endl;
				values[intp][inta]=values[intp][inta]+1;
				//if(p<0){cout<<xD<<","<<yD<<" -> "<<p<<"/"<<r<<" : "<<values[intp][inta]<<endl;}
				if(values[intp][inta]==3){
					//cout<<(intp/r)*CV_PI*2-CV_PI<<","<<inta*r-maxX/2<<" : "<<values[intp][inta]<<endl;
					in.push_back(Vec2f((intp/r)*CV_PI*2,inta/r-maxX/2));
				}
			}
			
    		}
	}
    }
}

/*
 * Main principal
 */
int main(int argc, char *argv[]) {
    char key=0;
    string image;
    if (argc == 1) {
        cout << "Introduza la ruta de la imagen;" << endl; 
        cin>> image;
    } else {
        image = argv[1];
    }

    Mat bgrMap,captura = imread(image, CV_LOAD_IMAGE_COLOR); //Carga la imagen recibida por parametro


   VideoCapture TheVideoCapturer(image.c_str());
    if (captura.empty()&&!TheVideoCapturer.isOpened()) 
        {
		TheVideoCapturer.open(atoi(image.c_str())); //Abre la videocamara
	}
   if (!TheVideoCapturer.isOpened()&&captura.empty()) {
 	std::cerr << "Could not open file " << image << std::endl;
        return -1;
    }
    if (!captura.empty()){
    	bgrMap=captura;
    }else{
    	TheVideoCapturer >> bgrMap;
    }


    namedWindow("Original image", CV_WINDOW_AUTOSIZE);
    namedWindow("Gx", CV_WINDOW_AUTOSIZE);
    namedWindow("Magnitud del Gradiente", CV_WINDOW_AUTOSIZE);

    while (key != 27 && (!captura.empty() || TheVideoCapturer.grab())) {
        if(captura.empty()){ //Se esta usando una camara
        	TheVideoCapturer >> bgrMap;
        } 
    imshow("Original image", bgrMap);


    GaussianBlur(bgrMap, bgrMap, Size(3, 3), 0, 0, BORDER_DEFAULT); //Filtro Gausiano

    Mat grey;
    cvtColor(bgrMap, grey, CV_BGR2GRAY);
    //Gradiente X/////////////////////////////////////
    Mat sobelx;
    Sobel(grey, sobelx, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT);

    double minValX, maxValX;
    minMaxLoc(sobelx, &minValX, &maxValX); //find minimum and maximum intensities
    //cout << "Gx" << "minVal : " << minValX << endl << "maxVal : " << maxValX << endl;

    Mat drawX;
    sobelx.convertTo(drawX, CV_8U, 255.0 / (maxValX - minValX), -minValX * 255.0 / (maxValX - minValX));

    imshow("Gx", drawX);
    //////////////////////////////////////////////////


    //Gradiente Y/////////////////////////////////////
    Mat sobely;
    Sobel(grey, sobely, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT);

    double minValY, maxValY;
    minMaxLoc(sobely, &maxValY, &minValY); //find minimum and maximum intensities
    //cout << "Gy" << "minVal : " << minValY << endl << "maxVal : " << maxValY << endl;

    Mat drawY;
    sobely.convertTo(drawY, CV_8U, 255.0 / (maxValY - minValY), -minValY * 255.0 / (maxValY - minValY));

    imshow("Gy", drawY);
    //////////////////////////////////////////////////


    //Magnitud del Gradiente//////////////////////////
    Mat grad;
    Mat abs_grad_x, abs_grad_y;
    convertScaleAbs(sobelx, abs_grad_x);
    convertScaleAbs(sobely, abs_grad_y);
    addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);

    imshow("Magnitud del Gradiente", grad);
    //////////////////////////////////////////////////


    //Direcion del gradiente//////////////////////////
    Mat dir=Mat::zeros(sobelx.size(),sobelx.type());
    Mat dirS=Mat::zeros(grey.size(),grey.type());
    float value;

    for(int i=0;i<grey.rows;i++){
    	for(int o=0;o<grey.cols;o++){
		value=M_PI-atan2(-sobely.at<float>(i,o),-sobelx.at<float>(i,o));
		value=value<0?0:value;  //Solucion para valores negativos muy pequeños
		dir.at<float>(i,o)=value;
		dirS.at<uchar>(i,o)=value/M_PI*128;
	}
    }

    imshow("Direccion del Gradiente", dirS);
    //////////////////////////////////////////////////


    //Votacion al punto central///////////////////////

    /*
    Mat votos=bgrMap.clone();
    Mat fuga=bgrMap.clone();
    Vec3b color=Vec3b(255,0,0);
    float angle,xD,yD,xP,yP;
    int mean=round(grey.rows/2); //Horizonte 
    int minY=round(mean-(grey.rows/2)*MARGEN);  //
    int maxY=round(mean+(grey.rows/2)*MARGEN);

    int values[bgrMap.cols][maxY-minY];  //tabla de votos
    for(int i=0;i<bgrMap.cols;i++){
        for(int o=0;o<maxY-minY;o++){
    		values[i][o]=0;
	}
    }

    int direcion=0; 
    int max=0,maxIX=0,maxIY=0;
    for(int i=0;i<grad.cols;i++){
    	for(int o=0;o<grad.rows;o++){
    		xP=i;		 //Posicion inicial en el eje X
		yP=o;            //Posicion inicial en el eje Y
    		if((int)grad.at<uchar>(yP,xP)>UMBRAL){
    			angle=dir.at<float>(yP,xP);
			xD=sin(angle);   //Modulo de la direcion en el eje X
			yD=cos(angle);   //Modulo de la direcion en el eje Y
			if(abs(yD)<0.9&&abs(yD)>0.1){    //Se eliminan las lineas verticales
				direcion=(yP<minY&&yD>0)||(yP>minY&&yD<0)?1:-1;   // direcion del vector
				int intX=round(xP);
				int intY=round(yP);
				while(intY!=minY
					&&(intX>=0&&intX<bgrMap.cols)&&(intY>=0&&intY<bgrMap.rows)){
					votos.at<Vec3b>(yP,xP)=color;
					xP=xP+direcion*xD;    //avanza
					yP=yP+direcion*yD;
					intX=round(xP);
					intY=round(yP);
				}

				direcion=(yP<maxY&&yD>0)||(yP>maxY&&yD<0)?1:-1;   // direcion del vector
				while((intX>=0&&intX<bgrMap.cols)&&(intY>=minY&&intY<maxY)){ 
							//Comprueba que el punto central se encuentra en la imagen
					int valueY=intY-minY;
					values[intX][valueY]=values[intX][valueY]+1;  //Vota al pixel
					if(values[intX][valueY]>max){    //Pixel mas votado?
						max=values[intX][valueY];
						maxIX=intX;
						maxIY=intY;
					}
					votos.at<Vec3b>(yP,xP)=color;
					xP=xP+direcion*xD;    //avanza
					yP=yP+direcion*yD;
					intX=round(xP);
					intY=round(yP);
				}
					
			}
    		}
	}
    }

    //Dibuja la cruz
    for(int i=0;i<bgrMap.rows;i++){
    	fuga.at<Vec3b>(i,maxIX)[0]=0;
	fuga.at<Vec3b>(i,maxIX)[1]=255;
	fuga.at<Vec3b>(i,maxIX)[2]=0;	
    }
    for(int i=0;i<bgrMap.cols;i++){
    	fuga.at<Vec3b>(maxIY,i)[0]=0;
	fuga.at<Vec3b>(maxIY,i)[1]=255;
	fuga.at<Vec3b>(maxIY,i)[2]=0;	
    }

    imshow("Punto central", votos);
    imshow("Punto central max", fuga);
    */
   
 
    /*
     * Metodo de Hough 
     */
    Mat dst;
    Mat votos=bgrMap.clone();
    Mat fuga=bgrMap.clone();
    float angle,xD,yD,xP,yP;
    int mean=round(grey.rows/2); //Horizonte 
    int minY=round(mean-(grey.rows/2)*MARGEN);  //
    int maxY=round(mean+(grey.rows/2)*MARGEN);

    int values[bgrMap.cols][maxY-minY];  //tabla de votos
    for(int i=0;i<bgrMap.cols;i++){
        for(int o=0;o<maxY-minY;o++){
    		values[i][o]=0;
	}
    }

    int direcion=0; 
    int max=0,maxIX=0,maxIY=0;

    Canny(grey, dst, 50, 200, 3);

    vector<Vec2f> lines;
    HoughLines(dst, lines, 1, CV_PI/180, UMBRAL, 0, 0 );
    //getLines(lines, grad,dir,CV_PI/180, 1000);
     for( size_t i = 0; i < lines.size(); i++ )
     {
     	float rho = lines[i][0], theta = lines[i][1];

    	double yD = cos(theta), xD = -sin(theta);
	if(abs(yD)<0.9&&abs(yD)>0.1){    //Se eliminan las lineas verticales
		Point pt1, pt2;
    		double xP = yD*rho, yP = -xD*rho;
 	   	pt1.x = cvRound(xP + 1000*(xD));
	    	pt1.y = cvRound(yP + 1000*(yD));
	     	pt2.x = cvRound(xP - 1000*(xD));
     		pt2.y = cvRound(yP - 1000*(yD));
     		line( votos, pt1, pt2, Scalar(255,0,0), 1, CV_AA);

	
		//minY=(yP+sin(theta))*k
		int k=(minY-yP)/yD;
		xP=(xP+xD*k);
		yP=minY;
		int intX=round(xP);
		int intY=round(yP);
		direcion=(yP<maxY&&yD>0)||(yP>maxY&&yD<0)?1:-1;   // direcion del vector
		while((intY>=minY&&intY<maxY)){ 
			if((intX>=0&&intX<bgrMap.cols)){
				int valueY=intY-minY;
				votos.at<Vec3b>(yP,xP)=Vec3b(0,255,0);
				values[intX][valueY]=values[intX][valueY]+1;  //Vota al pixel
				if(values[intX][valueY]>max){    //Pixel mas votado?
					max=values[intX][valueY];
					maxIX=intX;
					maxIY=intY;
				}
			}
			xP=xP+direcion*xD;    //avanza
			yP=yP+direcion*yD;
			intX=round(xP);
			intY=round(yP);
		}
	}
    }
    //Dibuja la cruz
    for(int i=0;i<bgrMap.rows;i++){
    	fuga.at<Vec3b>(i,maxIX)[0]=0;
	fuga.at<Vec3b>(i,maxIX)[1]=255;
	fuga.at<Vec3b>(i,maxIX)[2]=0;	
    }
    for(int i=0;i<bgrMap.cols;i++){
    	fuga.at<Vec3b>(maxIY,i)[0]=0;
	fuga.at<Vec3b>(maxIY,i)[1]=255;
	fuga.at<Vec3b>(maxIY,i)[2]=0;	
    }

    imshow("Punto central", votos);
    imshow("Punto central max", fuga);
    //////////////////////////////////////////////////
    if(!captura.empty()){while(key != 27 ){key = waitKey(20);}return 0;}
        key = waitKey(20);
}

    return 0;
} 

