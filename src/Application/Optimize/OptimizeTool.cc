#include <iostream>

#include <QXmlStreamWriter>
#include <QTemporaryFile>
#include <QFileDialog>
#include <QThread>
#include <QMessageBox>
#include <Optimize/OptimizeTool.h>
#include <Visualization/ShapeworksWorker.h>
#include <Data/Project.h>
#include <Data/Mesh.h>
#include <Data/Shape.h>

#include <ui_OptimizeTool.h>

//---------------------------------------------------------------------------
OptimizeTool::OptimizeTool(Preferences& prefs) : preferences_(prefs) {
  this->ui_ = new Ui_OptimizeTool;
  this->ui_->setupUi( this );
  this->setupTable(this->ui_->number_of_scales->value());
}

//---------------------------------------------------------------------------
OptimizeTool::~OptimizeTool()
{}

//---------------------------------------------------------------------------
void OptimizeTool::on_export_xml_button_clicked()
{
  std::cerr << "Export XML\n";

  QString filename = QFileDialog::getSaveFileName( this, tr( "Save as..." ),
                                                   QString(), tr( "XML files (*.xml)" ) );
  if ( filename.isEmpty() ) {
    return;
  }

  this->export_xml( filename );
}

//---------------------------------------------------------------------------
void OptimizeTool::handle_error(std::string msg) {
  emit error_message(msg);
}

//---------------------------------------------------------------------------
void OptimizeTool::handle_progress(int val) {
  emit progress(static_cast<size_t>(val));
}

//---------------------------------------------------------------------------
void OptimizeTool::on_run_optimize_button_clicked() {
  this->update_preferences();
  emit message("Please wait: running optimize step...");
  emit progress(5);
  this->ui_->run_optimize_button->setEnabled(false);
  auto shapes = this->project_->get_shapes();
  std::vector<ImageType::Pointer> imgs;
  for (auto s : shapes) {
    imgs.push_back(s->get_groomed_image());
  }
  auto scales = this->ui_->number_of_scales->value();
  this->optimize_ = new ShapeWorksOptimize(this, imgs, scales,
    this->getStartRegs(), this->getEndRegs(), this->getIters(),
    this->getTolerances(), this->getDecaySpans(), true);

  QThread *thread = new QThread;
  ShapeworksWorker *worker = new ShapeworksWorker(
    ShapeworksWorker::Optimize, NULL, this->optimize_, this->project_);
  worker->moveToThread(thread);
  connect(thread, SIGNAL(started()), worker, SLOT(process()));
  connect(worker, SIGNAL(result_ready()), this, SLOT(handle_thread_complete()));
  connect(this->optimize_, SIGNAL(progress(int)), this, SLOT(handle_progress(int)));
  connect(worker, SIGNAL(error_message(std::string)), this, SLOT(handle_error(std::string)));
  connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
  thread->start();
}

void OptimizeTool::on_number_of_scales_valueChanged(int val) {
  this->setupTable(val);
}

//---------------------------------------------------------------------------
void OptimizeTool::handle_thread_complete() {
  auto local = this->optimize_->localPoints();
  auto global = this->optimize_->globalPoints();
  this->project_->load_points(local, true);
  this->project_->load_points(global, false);
  this->project_->calculate_reconstructed_samples();
  emit progress(100);
  emit message("Optimize Complete");
  emit optimize_complete();
  this->ui_->run_optimize_button->setEnabled(true);
}

//---------------------------------------------------------------------------
bool OptimizeTool::export_xml( QString filename )
{
  std::cerr << "export to " << filename.toStdString() << "\n";

  QFile file( filename );

  if ( !file.open( QIODevice::WriteOnly ) )
  {
    QMessageBox::warning( 0, "Read only", "The file is in read only mode" );
    return false;
  }

  QSharedPointer<QXmlStreamWriter> xml_writer = QSharedPointer<QXmlStreamWriter>( new QXmlStreamWriter() );
  xml_writer->setAutoFormatting( true );
  xml_writer->setDevice( &file );
  xml_writer->writeStartDocument();

  /*xml_writer->writeTextElement( "number_of_particles",
                                QString::number( this->ui_->number_of_scales->value() ) );

  xml_writer->writeTextElement( "starting_regularization",
                                QString::number( this->ui_->starting_regularization_->value() ) );

  xml_writer->writeTextElement( "ending_regularization",
                                QString::number( this->ui_->ending_regularization_->value() ) );

  xml_writer->writeTextElement( "optimization_iterations",
                                QString::number( this->ui_->optimization_iterations_->value() ) );

  xml_writer->writeTextElement( "tolerance",
                                QString::number( this->ui_->tolerance->value() ) );
*/
  QVector<QSharedPointer<Shape> > shapes = this->project_->get_shapes();
  QFileInfo fi( shapes[0]->get_original_filename_with_path() );
  QString project_path = fi.dir().absolutePath();
  std::string name = this->project_->get_shapes()[0]->get_original_filename().toStdString();
  name = name.substr(0,name.size() - 5);
  xml_writer->writeTextElement( "output_points_prefix",
	  project_path + QDir::separator() + QString::fromStdString(name) );

  // inputs
  xml_writer->writeStartElement( "inputs" );
  xml_writer->writeCharacters( "\n" );

  for ( int i = 0; i < shapes.size(); i++ )
  {
    xml_writer->writeCharacters( shapes[i]->get_groomed_filename_with_path() + "\n" );
  }
  xml_writer->writeEndElement(); // inputs

  xml_writer->writeEndDocument();

  return true;
}

//---------------------------------------------------------------------------
void OptimizeTool::set_project( QSharedPointer<Project> project )
{
  this->project_ = project;
}

void OptimizeTool::setupTable(int rows) {
  auto table = this->ui_->parameterTable;
  table->setRowCount(rows);
  table->setColumnCount(5);
  QStringList Hheader, Vheader;
  Hheader << "StartReg" << "EndReg"
    << "Iters" << "Toler." << "DecaySpan";
  table->setHorizontalHeaderLabels(Hheader);
  for (size_t i = 0; i < rows; i++) {
    Vheader << QString::number(i + 1);
  }
  this->set_preferences(false);
  table->verticalHeader()->setVisible(true);
  table->setVerticalHeaderLabels(Vheader);
  table->setEditTriggers(QAbstractItemView::DoubleClicked);
  table->setSelectionMode(QAbstractItemView::SingleSelection);
  table->setShowGrid(true);
  table->setStyleSheet("QTableView {selection-background-color: 0x1111;}");
  QString styleSheet = "::section {"
    "spacing: 0px;"
    "background-color: 0xEEEE;"
    "color: black;"
    "border: 1px solid black;"
    "margin: 0px;"
    "text-align: left;"
    "font-family: arial;"
    "font-size: 11px; }";
  table->horizontalHeader()->setStyleSheet(styleSheet);
  table->verticalHeader()->setStyleSheet(styleSheet);
  table->resizeColumnsToContents();
  table->resizeRowsToContents();
}

std::vector<double> OptimizeTool::getStartRegs() {
  auto ans =  std::vector<double>();
  auto table = this->ui_->parameterTable;
  for (size_t i = 0; i < table->rowCount(); i++) {
    ans.push_back(table->item(i, 0)->text().toDouble());
  }
  return ans;
}

std::vector<double> OptimizeTool::getEndRegs() {
  auto ans = std::vector<double>();
  auto table = this->ui_->parameterTable;
  for (size_t i = 0; i < table->rowCount(); i++) {
    ans.push_back(table->item(i, 1)->text().toDouble());
  }
  return ans;
}

std::vector<double> OptimizeTool::getDecaySpans() {
  auto ans = std::vector<double>();
  auto table = this->ui_->parameterTable;
  for (size_t i = 0; i < table->rowCount(); i++) {
    ans.push_back(table->item(i, 4)->text().toDouble());
  }
  return ans;
}

std::vector<double> OptimizeTool::getTolerances() {
  auto ans = std::vector<double>();
  auto table = this->ui_->parameterTable;
  for (size_t i = 0; i < table->rowCount(); i++) {
    ans.push_back(table->item(i, 3)->text().toDouble());
  }
  return ans;
}

std::vector<unsigned int> OptimizeTool::getIters() {
  auto ans = std::vector<unsigned int>();
  auto table = this->ui_->parameterTable;
  for (size_t i = 0; i < table->rowCount(); i++) {
    ans.push_back(table->item(i, 2)->text().toUInt());
  }
  return ans;
}

void OptimizeTool::set_preferences(bool setScales) {
  if (setScales) {
    this->ui_->number_of_scales->setValue(
      this->preferences_.get_preference("groom_scales",
        this->ui_->number_of_scales->value()));
  }
  auto table = this->ui_->parameterTable;
  auto rows = this->ui_->number_of_scales->value();
  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < 5; j++) {
      QString cellname;
      switch (j) {
      case 0:
        cellname = "groom_start_reg";
        break;
      case 1:
        cellname = "groom_end_reg";
        break;
      case 2:
        cellname = "groom_iters";
        break;
      case 3:
        cellname = "groom_tolerance";
        break;
      case 4:
        cellname = "groom_decay_span";
        break;
      }
      auto defaultVal = this->preferences_.get_preference(cellname.toStdString(), j == 2 ? 0 : 0.);
      auto cellTest = cellname + QString::number(i);
      auto cellVal = this->preferences_.get_preference(cellTest.toStdString(), defaultVal);
      table->setItem(i, j, new QTableWidgetItem(QString::number(cellVal)));
    }
  }
}

void OptimizeTool::update_preferences() {
  this->preferences_.set_preference("groom_scales",
      this->ui_->number_of_scales->value());
  auto table = this->ui_->parameterTable;
  auto rows = this->ui_->number_of_scales->value();
  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < 5; j++) {
      QString cellname;
      switch (j) {
      case 0:
        cellname = "groom_start_reg";
        break;
      case 1:
        cellname = "groom_end_reg";
        break;
      case 2:
        cellname = "groom_iters";
        break;
      case 3:
        cellname = "groom_tolerance";
        break;
      case 4:
        cellname = "groom_decay_span";
        break;
      }
      auto cellTest = cellname + QString::number(i);
      this->preferences_.set_preference(cellTest.toStdString(),
        j == 2 ? table->item(i, j)->text().toUInt() :
        table->item(i, j)->text().toDouble());
    }
  }
}
