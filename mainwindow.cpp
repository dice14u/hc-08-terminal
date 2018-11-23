#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QPushButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    worker(new HCWorker(this))
{
    this->setWindowTitle("HC-Terminal");
    ui->setupUi(this);

    editor = ui->term;
    input = ui->input;
    button = ui->send;
    bool recieving = false;

    connect(worker, &HCWorker::message, this, [this, &recieving](QString message) {
        if (!recieving) {
            editor->insertPlainText("\nRecieved: ");
            recieving = true;
        }

        editor->insertPlainText(message);
    });

    connect(button, &QPushButton::pressed, this, [this, &recieving]() {
        recieving = false;
        QString message = input->toPlainText();
        worker->writeData(message + "\r\n");
        editor->append("Sent: " + message);
        input->setPlainText("");
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    delete worker;
}
