#include "radar_window.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  RadarWindow window;
  window.show();

  // // Create the window
  // Window window;
  // window.show();

  // // Call the window.timerEvent every 40 ms
  // window.startTimer(40);

  // Execute the application
  return app.exec();


}