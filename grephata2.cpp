#include <iostream>
#include <algorithm>
#include <iterator>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <X11/Xutil.h>
#include <X11/Xlib.h>

using namespace std;

// Custom datatype containing the rgb values
typedef struct rgbContainer{
	int r;
	int g;
	int b;
}rgbContainer;

typedef struct spotLight{
	int posX;
	int posY;
	int radius;
	double intensity;
	int colR;
	int colG;
	int colB;
}spotLight;

char WINDOW_NAME[] = "Graphics Window";
char ICON_NAME[] = "Icon";
Display *display;
int screen;
Window main_window;
GC gc;
unsigned long foreground, background;
Status rc;
Colormap screen_colormap;
XColor red, blue,green;

int xs[] = {100, 700, 700, 100};
int ys[] = {100, 100, 550, 550};

string lastKey = "1";
bool rememberMode = false;
bool randomMode = false;
bool oneTriangleDraw = false;
bool noClick = false;

int edgePointNum = 0;
int edgeStartPoint = 0;
int xEdgePoints[20000];
int yEdgePoints[20000];

bool canPaint = false;
int pointNum = 0;
int startPoint = 0;
int startPointNum = 0;
int startPoints[20000];
int xPoints[2000000];
int yPoints[2000000];

int allPointNum = 0;
int numOfColors = 0;
int allXPoints[20000000];
int allYPoints[20000000];

rgbContainer triangleColors[20000];
spotLight lights[10];


void connectX()
{
   display = XOpenDisplay(NULL);
   if (display == NULL) {fprintf(stderr, "Cannot connect to X\n");
                         exit(1);}
   screen = DefaultScreen(display);
   foreground = BlackPixel(display, screen);
   background = WhitePixel(display, screen);
}

Window openWindow(int x, int y, int width, int height,
                  int border_width, int argc, char** argv)
{
    Window new_window;
    XSizeHints size_hints;
    new_window = XCreateSimpleWindow(display, DefaultRootWindow(display),
                 x, y, width, height, border_width, foreground,
                 background);
   size_hints.x = x;
   size_hints.y = y;
   size_hints.width = width;
   size_hints.height = height;
   size_hints.flags = PPosition | PSize;
   XSetStandardProperties(display, new_window, WINDOW_NAME, ICON_NAME,
                          None, argv, argc, &size_hints);
   XSelectInput(display, new_window, (ButtonPressMask | KeyPressMask |
                                      ExposureMask | PointerMotionMask));
   return (new_window);
}

GC getGC()
{
    GC gc;
    XGCValues gcValues;
    gc = XCreateGC(display, main_window, (unsigned long)0, &gcValues);
    XSetBackground(display, gc, background);
    XSetForeground(display, gc, foreground);
    return (gc);
}

void disconnectX()
{
   XCloseDisplay(display);
   exit(0);
}

void setColor(int red, int green, int blue){
   XColor col;

   col.pixel = ((blue & 0xff) | ((green & 0xff) << 8) | ((red & 0xff) << 16));

   XSetForeground(display, gc,col.pixel);
}

void CustomDrawLine(int x1, int y1, int x2, int y2, bool paint)
{
	/*
	// Set a random color for the triangle
	srand(time(NULL));
	int red = rand() % 255 + 1;
	int green = rand() % 255 + 1;
	int blue = rand() % 255 + 1;

	// Save the triangle color information with the point where the color is changed
	colorDivisors[numOfColors] = allPointNum;
	triangleColors[numOfColors].r = red;
	triangleColors[numOfColors].g = green;
	triangleColors[numOfColors].b = blue;
	numOfColors++;

	setColor(red, green, blue);
	*/

    int dx = abs(x2 - x1);
    int sx = x1 < x2 ? 1 : -1;

    int dy = -abs(y2 - y1);
    int sy = y1 < y2 ? 1 : -1;

    int err = dx + dy;
    int e2;

    for (;;)
    {
    	if (paint == true)
    		XDrawPoint(display,main_window, gc, x1, y1);

        bool contains = false;

        for (int i = startPoint + 1; i <= pointNum; i++)
        {
        	// Prevent multiple points on the x axis from being stored
        	//		as it messes up the filling algorithm
        	if (yPoints[pointNum - 1] == y1)
        	{
        		contains = true;
        		break;
        	}

        	if (xPoints[i] == x1 && yPoints[i] == y1)
        	{
        		contains = true;
        		break;
        	}
        }

        // Preserve the edge line values
        if (contains == false)
        {
        	xPoints[pointNum] = x1;
        	yPoints[pointNum] = y1;

        	pointNum++;
        }

        if (x1 == x2 && y1 == y2)
        {
            break;
        }

        e2 = 2 * err;

        // If the error > 0
        if (e2 >= dy)
        {
            err += dy; x1 += sx;
        }

        // If the error < 0
        if (e2 <= dx)
        {
            err += dx; y1 += sy;
        }
    }
}

void GetColorForPixel(int x, int y)
{
	int distTo0 = 0;
	int distTo1 = 0;
	int distTo2 = 0;

	if (rememberMode == false)
	{
		distTo0 = sqrt(pow(x - xEdgePoints[edgeStartPoint + 0], 2) + pow(y - yEdgePoints[edgeStartPoint + 0], 2));
		distTo1 = sqrt(pow(x - xEdgePoints[edgeStartPoint + 1], 2) + pow(y - yEdgePoints[edgeStartPoint + 1], 2));
		distTo2 = sqrt(pow(x - xEdgePoints[edgeStartPoint + 2], 2) + pow(y - yEdgePoints[edgeStartPoint + 2], 2));
	}
	else // Colors depening only the first selected 3 colors
	{
		distTo0 = sqrt(pow(x - xEdgePoints[0], 2) + pow(y - yEdgePoints[0], 2));
		distTo1 = sqrt(pow(x - xEdgePoints[1], 2) + pow(y - yEdgePoints[1], 2));
		distTo2 = sqrt(pow(x - xEdgePoints[2], 2) + pow(y - yEdgePoints[2], 2));
	}

	int totalDist = distTo0 + distTo1 + distTo2;

	double ratioFor0 = 0.0;
	double ratioFor1 = 0.0;
	double ratioFor2 = 0.0;

	if (lastKey == "1") // Normal Coloring mode
	{
		ratioFor0 =  (totalDist - distTo0) / (1.0 * totalDist);
		ratioFor1 =  (totalDist - distTo1) / (1.0 * totalDist);
		ratioFor2 =  (totalDist - distTo2) / (1.0 * totalDist);
	}
	else if (lastKey == "2")
	{
		ratioFor0 =  totalDist - distTo0 / (1.0 * totalDist);
		ratioFor1 =  totalDist - distTo1 / (1.0 * totalDist);
		ratioFor2 =  totalDist - distTo2 / (1.0 * totalDist);
	}
	else if (lastKey == "3")
	{
		ratioFor0 =  (totalDist - distTo0) / 1.0 * totalDist;
		ratioFor1 =  (totalDist - distTo1) / 1.0 * totalDist;
		ratioFor2 =  (totalDist - distTo2) / 1.0 * totalDist;
	}
	else if (lastKey == "4")
	{
		ratioFor0 =  (totalDist - distTo0) / 1.0 * totalDist;
		ratioFor1 =  (totalDist - distTo1) / (1.0 * totalDist);
		ratioFor2 =  (totalDist - distTo2) / (1.0 * totalDist);
	}
	else if (lastKey == "5")
	{
		ratioFor0 =  (totalDist - distTo0) / (1.0 * totalDist);
		ratioFor1 =  (totalDist - distTo1) / 1.0 * totalDist;
		ratioFor2 =  (totalDist - distTo2) / (1.0 * totalDist);
	}
	else if (lastKey == "6")
	{
		ratioFor0 =  (totalDist - distTo0) / (1.0 * totalDist);
		ratioFor1 =  (totalDist - distTo1) / (1.0 * totalDist);
		ratioFor2 =  (totalDist - distTo2) / 1.0 * totalDist;
	}

	float red = 1;
	float green = 1;
	float blue = 1;

	if (rememberMode == true)
	{
		srand(time(NULL));
		red = (float)(rand() % 255 + 1) / 255;
		green = (float)(rand() % 255 + 1) / 255;
		blue = (float)(rand() % 255 + 1) / 255;
	}
	else
	{
		red = 1;
		green = 1;
		blue = 1;
	}

	double rSpotLight[10];
	double gSpotLight[10];
	double bSpotLight[10];

	double rFinalSpotLight = 1;
	double gFinalSpotLight = 1;
	double bFinalSpotLight = 1;

	int size = 3;

	for (int i = 0; i < size; i++)
	{
		int pointToLightCenter = sqrt(pow(x - lights[i].posX, 2) + pow(y - lights[i].posY, 2));
		double distanceRatio = 1.0 * pointToLightCenter / (1.8 * lights[i].radius);

		if (pointToLightCenter < lights[i].radius)
		{
			rSpotLight[i] = (1 - distanceRatio) * lights[i].intensity * 1.0 * lights[i].colR / (1.0 * 255);
			gSpotLight[i] = (1 - distanceRatio) * lights[i].intensity * 1.0 * lights[i].colG / (1.0 * 255);
			bSpotLight[i] = (1 - distanceRatio) * lights[i].intensity * 1.0 * lights[i].colB / (1.0 * 255);

			//rFinalSpotLight = (1 - distanceRatio) * lights[i].intensity * 1.0 * lights[i].colR / (1.0 * 255);
			//gFinalSpotLight = (1 - distanceRatio) * lights[i].intensity * 1.0 * lights[i].colR / (1.0 * 255);
			//bFinalSpotLight = (1 - distanceRatio) * lights[i].intensity * 1.0 * lights[i].colR / (1.0 * 255);

			rFinalSpotLight += rSpotLight[i];
			gFinalSpotLight += gSpotLight[i];
			bFinalSpotLight += bSpotLight[i];
		}
	}

	rFinalSpotLight = rFinalSpotLight / (1.0 * size);
	gFinalSpotLight = gFinalSpotLight / (1.0 * size);
	bFinalSpotLight = bFinalSpotLight / (1.0 * size);

	setColor(rFinalSpotLight * ratioFor0 * 255, gFinalSpotLight * ratioFor1 * 255, bFinalSpotLight * ratioFor2 * 255);
}

void CustomFillPolygon(int minX, int maxX, int minY, int maxY, bool needtoAdd)
{
	lights[0].posX = 200;
	lights[0].posY = 340;
	lights[0].radius = 150;
	lights[0].intensity = 0.8;
	lights[0].colR = 255;
	lights[0].colG = 250;
	lights[0].colB = 6;

	lights[1].posX = 350;
	lights[1].posY = 300;
	lights[1].radius = 200;
	lights[1].intensity = 0.8;
	lights[1].colR = 255;
	lights[1].colG = 100;
	lights[1].colB = 50;

	lights[2].posX = 150;
	lights[2].posY = 150;
	lights[2].radius = 50;
	lights[2].intensity = 0.4;
	lights[2].colR = 100;
	lights[2].colG = 180;
	lights[2].colB = 10;

	int canPaint = true;
	// For each line
	for (int y = minY + 1; y <= maxY - 1; y++)
	{
		int pointsInLine = 0;

		for (int k = startPoint; k <= pointNum; k++)
		{
			if (y == yPoints[k])
			{
				pointsInLine++;
			}
		}

		canPaint = false;
		// For each pixel of the line
		//		check whether the point is in the xEdgePoint array
		//		if it is, check if the pixel is in the correct line by checking whether
		//		the value of xEdgePoint[index of xEdgePoint] = i
		for (int x = minX; x <= maxX; x++)
		{
			// Loops through our x values to see if it exists
			for (int k = startPoint + 1; k <= pointNum; k++)
			{
				if (x == xPoints[k] && y == yPoints[k])
				{
					canPaint = !canPaint;
					break;
				}

				if (x == xPoints[k] && y == yPoints[k] && x == xPoints[k-1] && y == yPoints[k-1])
				{
					canPaint = false;
					break;
				}
			}

			if (pointsInLine == 1)
			{
				canPaint = false;
				//canPaint = !canPaint;
			}

			if (canPaint == true)
			{
				if (x > xs[0] && x < xs[2] && y > ys[0] && y < ys[2])
				{
					GetColorForPixel(x, y);
					XDrawPoint(display,main_window, gc, x, y);
				}

				if (needtoAdd == true)
				{
					// Preserve all the triangle fill points
					allXPoints[allPointNum] = x;
					allYPoints[allPointNum] = y;
					allPointNum++;
				}
			}

		}

/*
		// Find the slopes
		double m0 = 1.0 * ( yEdgePoints[1] - yEdgePoints[0]) / (1.0 * xEdgePoints[1] - xEdgePoints[0]);
		double m1 = 1.0 * (yEdgePoints[2] - yEdgePoints[1]) / (1.0 * xEdgePoints[2] - xEdgePoints[1]);
		double m2 = 1.0 * (yEdgePoints[0] - yEdgePoints[2]) / (1.0 * xEdgePoints[0] - xEdgePoints[2]);

		// Find the intersection point in the x axis with the infinite lines of the triangle
		int x0 = (y - yEdgePoints[0] + m0 * xEdgePoints[0]) / m0;
		int x1 = (y - yEdgePoints[1] + m1 * xEdgePoints[1]) / m1;
		int x2 = (y - yEdgePoints[2] + m2 * xEdgePoints[2]) / m2;

		int xIntersect1 = -1;
		int xIntersect2 = -1;
		int noOfIntersections = 0;

		// Find the actual intersection points in the x axis
		if ((x0 >= xEdgePoints[0] && x0 < xEdgePoints[1]) || ((x0 <= xEdgePoints[0] && x0 > xEdgePoints[1])))
		{
			xIntersect1 = x0;
			noOfIntersections++;
		}

		if ((x1 >= xEdgePoints[1] && x1 < xEdgePoints[2]) || (x1 <= xEdgePoints[1] && x1 > xEdgePoints[2]))
		{
			xIntersect2 = x1;
			noOfIntersections++;
		}

		if ((x2 >= xEdgePoints[2] && x2 < xEdgePoints[0]) || (x2 <= xEdgePoints[2] && x2 > xEdgePoints[0]))
		{
			if (xIntersect1 == -1 && xIntersect2 == -1)
			{
				canPaint = false;
			}
			else if (xIntersect1 == -1)
			{
				xIntersect1 = x2;
			}
			else if (xIntersect2 == -1)
			{
				xIntersect2 = x2;
			}

			noOfIntersections++;
		}

		if (noOfIntersections == 2)
		{
			canPaint = true;
		}

		if (xIntersect1 == -1 || xIntersect2 == -1)
		{
			canPaint = false;
		}

		if (canPaint == true)
		{
			// Swap the intersection points if necessary
			if (xIntersect1 > xIntersect2)
			{
				int temp = xIntersect1;
				xIntersect1 = xIntersect2;
				xIntersect2 = temp;
			}

			for (int x = xIntersect1; x <= xIntersect2; x++)
			{
				GetColorForPixel(x, y);
				XDrawPoint(display,main_window, gc, x, y);

				// Preserve all the triangle fill points
				allXPoints[allPointNum] = x;
				allYPoints[allPointNum] = y;
				allPointNum++;
			}
		}
		*/
	}
}

void CustomDraw()
{

}

void DoPolygonThing(bool needToAdd)
{
	int minX = 100000;
	int maxX = 0;
	int minY = 100000;
	int maxY = 0;

	for (int i = edgeStartPoint; i < edgePointNum; i++)
	{
		if (xEdgePoints[i] < minX)
			minX = xEdgePoints[i];

		if (xEdgePoints[i] > maxX)
			maxX = xEdgePoints[i];

		if (yEdgePoints[i] < minY)
			minY = yEdgePoints[i];

		if (yEdgePoints[i] > maxY)
			maxY = yEdgePoints[i];
	}

	CustomFillPolygon(minX, maxX, minY, maxY, needToAdd);

	if (needToAdd == true)
	{
		edgeStartPoint = edgePointNum;
		startPoint = pointNum;
		startPoints[startPointNum] = startPoint;
		startPointNum++;
	}
}

void doButtonPressEvent(XButtonEvent *pEvent)
{
  int x;
  int y;

  x = pEvent->x;
  y = pEvent->y;

  xEdgePoints[edgePointNum] = x;
  yEdgePoints[edgePointNum] = y;

  if (edgePointNum != edgeStartPoint)
  {
	  CustomDrawLine(xEdgePoints[edgePointNum - 1], yEdgePoints[edgePointNum - 1], xEdgePoints[edgePointNum], yEdgePoints[edgePointNum], false);
  }

  edgePointNum++;

  if (edgePointNum % 3 == 0)
  {
	  CustomDrawLine(xEdgePoints[edgeStartPoint], yEdgePoints[edgeStartPoint], xEdgePoints[edgePointNum - 1], yEdgePoints[edgePointNum - 1], false);

	  DoPolygonThing(!oneTriangleDraw);

	  if (oneTriangleDraw == true)
	  {
		  noClick = true;
	  }
  }
}

void doKeyPressEvent(XKeyEvent *pEvent)
{
    int key_buffer_size = 10;
    char key_buffer[9];
    XComposeStatus compose_status;
    KeySym key_sym;
    XLookupString(pEvent, key_buffer, key_buffer_size,
                  &key_sym, &compose_status);

    int moveAmount = 30;

    if (key_buffer[0] == 'q')
    {
    	disconnectX();
    }
    else if (key_buffer[0] == 'p')
	{
    	CustomDraw();
	}
    else if (key_buffer[0] == 'm')
	{
    	rememberMode = !rememberMode;
	}
    else if (key_buffer[0] == 'r')
	{
        randomMode = !randomMode;
	}
    else if (key_buffer[0] == '0')
	{
    	oneTriangleDraw = !oneTriangleDraw;
	}
    else if (key_buffer[0] == '1')
	{
        lastKey = "1";
    }
    else if (key_buffer[0] == '2')
	{
    	lastKey = "2";
	}
    else if (key_buffer[0] == '3')
	{
    	lastKey = "3";
	}
    else  if (key_buffer[0] == '4')
	{
    	lastKey = "4";
	}
    else if (key_buffer[0] == '5')
	{
    	lastKey = "5";
	}
    else if (key_buffer[0] == '6')
	{
    	lastKey = "6";
	}
    else if (key_buffer[0] == 'a')
	{
    	for(int i = 0; i < 4; i++)
    	{
    		xs[i] -= moveAmount;
    	}

    	for(int y = ys[0]; y <= ys[2]; y++)
    	{
    		for(int x = xs[2], x2 = xs[0]; x <= xs[2] + moveAmount; x++, x2++)
    		{
    	    	setColor(1 * 255, 1 * 255, 1 * 255);
    	    	XDrawPoint(display,main_window, gc, x, y);
    	    	XDrawPoint(display,main_window, gc, x2, y);
    		}
    	}

    	DoPolygonThing(false);
	}
    else if (key_buffer[0] == 'd')
    {
    	for(int i = 0; i < 4; i++)
    	{
    		xs[i] += moveAmount;
    	}

    	for(int y = ys[0]; y <= ys[2]; y++)
    	{
    		for(int x = xs[0] - moveAmount, x2 = xs[2] - moveAmount; x <= xs[0]; x++, x2++)
    		{
    			setColor(1 * 255, 1 * 255, 1 * 255);
    	    	XDrawPoint(display,main_window, gc, x, y);
    	    	XDrawPoint(display,main_window, gc, x2, y);
    		}
    	}

    	DoPolygonThing(false);
    }
    else if (key_buffer[0] == 'w')
	{
    	for(int i = 0; i < 4; i++)
    	{
    		ys[i] -= moveAmount;
    	}

    	for(int y = ys[2], y2 = ys[0]; y <= ys[2] + moveAmount; y++, y2++)
    	{
    		for(int x = xs[0]; x <= xs[2]; x++)
    	  	{
    	    	setColor(1 * 255, 1 * 255, 1 * 255);
    	    	XDrawPoint(display,main_window, gc, x, y);
    	    	XDrawPoint(display,main_window, gc, x, y2);
    		}
    	}

    	DoPolygonThing(false);
	}
    else if (key_buffer[0] == 's')
	{
    	for(int i = 0; i < 4; i++)
    	{
    		ys[i] += moveAmount;
		}
    	for(int y = ys[0] - moveAmount, y2 = ys[2] - moveAmount; y <= ys[0]; y++, y2++)
    	{
    		for(int x = xs[0]; x <= xs[2]; x++)
    		{
    			setColor(1 * 255, 1 * 255, 1 * 255);
    			XDrawPoint(display,main_window, gc, x, y);
    			XDrawPoint(display,main_window, gc, x, y2);
    		}
    	}

    	DoPolygonThing(false);
	}
}

// Call this function when the gui window is rescaled
void doExposeEvent(XExposeEvent *pEvent)
{
	DoPolygonThing(false);
}

void DrawViewBoundaries(int xs[], int ys[])
{
	setColor(0 * 255, 0 * 255, 0 * 255);
	XDrawLine(display, main_window, gc, xs[0], ys[0], xs[1], ys[1]);
	XDrawLine(display, main_window, gc, xs[1], ys[1], xs[2], ys[2]);
	XDrawLine(display, main_window, gc, xs[2], ys[2], xs[3], ys[3]);
	XDrawLine(display, main_window, gc, xs[3], ys[3], xs[0], ys[0]);
}

int main (int argc, char** argv)
{
	XEvent event;
	  connectX();
	  main_window = openWindow(10,20,800,600,5, argc, argv);
	  gc = getGC();
	  screen_colormap=DefaultColormap(display, DefaultScreen(display));
	  rc = XAllocNamedColor(display, screen_colormap, "red", &red, &red);
	  XSetForeground(display, gc, red.pixel);
	  XMapWindow(display, main_window);
	  XFlush(display);

    while (True)
    {
    	XNextEvent(display, &event);

    	switch (event.type)
    	{
    		case Expose:
    			doExposeEvent( (XExposeEvent*) &event);
    			break;
    		case ButtonPress:
    			if (noClick == false)
    			{
    				doButtonPressEvent( (XButtonEvent*) &event);
    			}
    	    	break;
    	    case KeyPress:
    	    	doKeyPressEvent( (XKeyEvent*) &event);
    	    	break;
    	}

    	DrawViewBoundaries(xs, ys);
    }

}





















