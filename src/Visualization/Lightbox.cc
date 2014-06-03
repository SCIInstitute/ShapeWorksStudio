#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <Visualization/Lightbox.h>
#include <Visualization/DisplayObject.h>
#include <Data/Mesh.h>
#include <Data/Shape.h>

//-----------------------------------------------------------------------------
Lightbox::Lightbox()
{
  this->renderer_ = vtkSmartPointer<vtkRenderer>::New();
  this->camera_ = this->renderer_->GetActiveCamera();

  this->tile_layout_width_ = 4;
  this->tile_layout_height_ = 4;
  this->start_row_ = 0;

  this->first_draw_ = true;
  this->auto_center_ = true;
}

//-----------------------------------------------------------------------------
Lightbox::~Lightbox()
{}

//-----------------------------------------------------------------------------
void Lightbox::set_interactor( vtkRenderWindowInteractor* interactor )
{
  this->interactor_ = interactor;
}

//-----------------------------------------------------------------------------
void Lightbox::insert_object_into_viewer( QSharedPointer<DisplayObject> object, int position )
{
  if ( position >= this->viewers_.size() )
  {
    return;
  }

  QSharedPointer<Viewer> viewer = this->viewers_[position];

  viewer->display_object( object, this->auto_center_ );

  if ( this->first_draw_ )
  {
    this->first_draw_ = false;
    viewer->reset_camera();
  }
}

//-----------------------------------------------------------------------------
void Lightbox::clear_renderers()
{
  for ( int i = 0; i < this->viewers_.size(); i++ )
  {
    this->viewers_[i]->get_renderer()->RemoveAllViewProps();
  }
}

//-----------------------------------------------------------------------------
void Lightbox::display_objects()
{
  this->clear_renderers();

  // skip based on scrollbar
  int start_mesh = this->start_row_ * this->tile_layout_width_;

  int position = 0;
  for ( int i = start_mesh; i < this->objects_.size(); i++ )
  {
    this->insert_object_into_viewer( this->objects_[i], position );
    position++;
  }

  this->render_window_->Render();
}

//-----------------------------------------------------------------------------
void Lightbox::set_render_window( vtkRenderWindow* renderWindow )
{
  this->render_window_ = renderWindow;
  this->render_window_->AddRenderer( this->renderer_ );

  this->setup_renderers();
}

//-----------------------------------------------------------------------------
void Lightbox::setup_renderers()
{
  for ( int i = 0; i < this->viewers_.size(); i++ )
  {
    this->render_window_->RemoveRenderer( this->viewers_[i]->get_renderer() );
  }

  this->viewers_.clear();

  int* size = this->render_window_->GetSize();

  int width = this->tile_layout_width_;
  int height = this->tile_layout_height_;
  int total = width * height;

  float margin = 0.005;

  float tile_height = ( 1.0f - ( margin * ( height + 1 ) ) ) / height;
  float tile_width = ( 1.0f - ( margin * ( width + 1 ) ) ) / width;

  float step_x = tile_width + margin;
  float step_y = tile_height + margin;

  for ( int y = 0; y < height; y++ )
  {
    for ( int x = 0; x < width; x++ )
    {
      int i = ( y * width ) + x;

      // create a renderer for the viewer
      vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
      renderer->SetActiveCamera( this->camera_ );

      // create the viewer
      QSharedPointer<Viewer> viewer = QSharedPointer<Viewer>( new Viewer() );
      viewer->set_renderer( renderer );

      this->viewers_.push_back( viewer );

      double viewport[4] = {0.0, 0.0, 0.0, 0.0};

      // horizontal
      viewport[0] = margin + ( x * step_x );
      viewport[2] = viewport[0] + tile_width;

      // vertical
      viewport[1] = margin + ( ( ( height - 1 ) - y ) * step_y );
      viewport[3] = viewport[1] + tile_height;

      renderer->SetViewport( viewport );

      double color = 0.2f;

      renderer->SetBackground( color, color, color );

      this->render_window_->AddRenderer( renderer );
    }
  }

  if ( this->render_window_->IsDrawable() )
  {
    this->render_window_->Render();
  }
}

//-----------------------------------------------------------------------------
void Lightbox::set_tile_layout( int width, int height )
{
  this->tile_layout_width_ = width;
  this->tile_layout_height_ = height;

  this->setup_renderers();
  this->display_objects();
}

//-----------------------------------------------------------------------------
int Lightbox::get_num_rows()
{
  return std::ceil( (float)this->objects_.size() / (float)this->tile_layout_width_ );
}

//-----------------------------------------------------------------------------
int Lightbox::get_num_rows_visible()
{
  return this->tile_layout_height_;
}

//-----------------------------------------------------------------------------
void Lightbox::set_start_row( int row )
{
  this->start_row_ = row;
  this->display_objects();
  this->render_window_->Render();
}

//-----------------------------------------------------------------------------
void Lightbox::set_auto_center( bool center )
{
  this->auto_center_ = center;
  this->first_draw_ = true;
}

//-----------------------------------------------------------------------------
void Lightbox::set_display_objects( QVector < QSharedPointer < DisplayObject > > objects )
{
  this->objects_ = objects;
  this->display_objects();
}