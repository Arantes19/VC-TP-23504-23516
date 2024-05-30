﻿#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}


void vc_timer(void) {
	static bool running = false;
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	if (!running) {
		running = true;
	}
	else {
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

		// Tempo em segundos.
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
		double nseconds = time_span.count();

		std::cout << "Tempo decorrido: " << nseconds << "segundos" << std::endl;
		std::cout << "Pressione qualquer tecla para continuar...\n";
		std::cin.get();
	}
}

int main(void) {
	// V�deo
	char videofile[20] = "video_resistors.mp4";
	cv::VideoCapture capture;
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;

	int contador=0;
	// Outros
	std::string str;
	int key = 0;

	/* Leitura de v�deo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video.avi dever� estar localizado no mesmo direct�rio que o ficheiro de c�digo fonte.
	*/
	capture.open(videofile);

	/* Em alternativa, abrir captura de v�deo pela Webcam #0 */
	//capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

	/* Verifica se foi poss�vel abrir o ficheiro de v�deo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de video!\n";
		return 1;
	}

	/* N�mero total de frames no v�deo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do v�deo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolu��o do v�deo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o v�deo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);
	cv::namedWindow("Segmented", cv::WINDOW_AUTOSIZE);

	/* Inicia o timer */
	vc_timer();

	cv::Mat frame;
	while (key != 'q') {
		/* Leitura de uma frame do v�deo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		cv::GaussianBlur(frame, frame, cv::Size(5, 5), 0); 

		/* N�mero da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		/* Exemplo de inser��o texto na frame */
		str = std::string("Resolucao: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("Total de Frames: ").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("Frame rate: ").append(std::to_string(video.fps));
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("N. da frame: ").append(std::to_string(video.nframe));
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		//Criação de imagens
		IVC *image0 = vc_image_new(video.width, video.height, 3, 255);

		//Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(image0->data, frame.data, video.width * video.height * 3);

		vc_convert_bgr_to_rgb(image0);

		IVC *image1 = vc_image_new(video.width, video.height, 3, 255);
		vc_rgb_to_hsv(image0, image1);

		IVC *image2 = vc_image_new(video.width, video.height, 1, 255); 
		vc_hsv_segmentation(image1, image2); //cor da resistencia

		IVC *image3 = vc_image_new(video.width, video.height, 1, 255);
		vc_binary_close(image2, image3, 3, 3);

		




		OVC *blobs = nullptr; 
		int nblobs = 0; 
		int count;
		int max_y;
		int min_y;
		int max_x;
		int min_x;

		IVC* image4 = vc_image_new(video.width, video.height, 1, 255);


		blobs = vc_binary_blob_labelling(image3, image4, &nblobs);

		vc_binary_blob_info(image4, blobs, nblobs);

		/*Foram realizados 3 desenhos de maneira a evitar a criação de um border entre 2 resistencias*/
		count = DRAW_RESISTOR_BOX_1(image4, image0, blobs, nblobs, video.width, video.height, &min_x, &max_x, &min_y, &max_y);//Parte de cima da imagem, <=30% da Height 
		DRAW_RESISTOR_BOX_2(image4, image0, blobs, nblobs, video.width, video.height, &min_x, &max_x, &min_y, &max_y);//Parte do meio da imagem, [30,70[% da Height
		DRAW_RESISTOR_BOX_3(image4, image0, blobs, nblobs, video.width, video.height, &min_x, &max_x, &min_y, &max_y);//Parte de baixo da imagem, >=70% da Height

		IVC* image5 = vc_image_new(video.width, video.height, 1, 255);
		vc_color_segmentation(image1, image5, max_y, min_y, max_x, min_x); //cores do Video

		IVC* image6 = vc_image_new(video.width, video.height, 1, 255);
		vc_binary_close(image5, image6, 3, 3);


		// Convert the binary image to color for visualization
		cv::Mat seg_frame(video.height, video.width, CV_8UC1, image6->data); 
		 
		cv::Mat color_seg_frame;
		cv::cvtColor(seg_frame, color_seg_frame, cv::COLOR_GRAY2BGR); 


		RGB_to_BGR(image0);  
		memcpy(frame.data, image0->data, video.width * video.height * 3); 

		str = std::string("Valor da resistencia: ").append(std::to_string(0));
		cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("Resistencias: ").append(std::to_string(count)); 
		cv::putText(frame, str, cv::Point(20, 150), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 150), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);


		//Liberta a memoria das imagens IVC criadas
		vc_image_free(image0);    
		vc_image_free(image1); 
		vc_image_free(image2);
		vc_image_free(image3);
		vc_image_free(image4);
		vc_image_free(image5);
		vc_image_free(image6);




		//vc_image_free(resistor_labeled); 
		free(blobs);  


		// Exibe a frame original
		cv::imshow("VC - VIDEO", frame);

		// Exibe a frame segmentada
		cv::imshow("Segmented", color_seg_frame);
		
		//int delay = 500 / video.fps;
		/* Sai da aplicacao, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}

	/* Para o timer e exibe o tempo decorrido */
	vc_timer();

	// Fecha as janelas
	cv::destroyWindow("VC - VIDEO");
	cv::destroyWindow("Segmented");

	/* Fecha o ficheiro de v�deo */
	capture.release();

	return 0;
}
