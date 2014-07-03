#ifndef PREFERENCES_HH
#define PREFERENCES_HH

#include <QWidget>
#include <QSettings>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalMapper>
#include <QPushButton>
#include <QFontDialog>
#include <QFileDialog>
#include <QIntValidator>
#include <QSpinBox>

class Preferences : public QWidget
{
  Q_OBJECT

public:

  enum GuiHint
  {
      SpinBox,
      LineEdit,
      FontDialog,
      FolderDialog
  };
  
  struct Item
  {
    Item( const QVariant& d=QVariant(), GuiHint h=LineEdit, QValidator* v=0)
            : data(d)
            , hint(h)
            , valid(v)
          {}
    
    QVariant data;
    GuiHint  hint;
    QValidator* valid;
  };

  Preferences() 
          : QWidget()
          , _s(this)
          , _sm_font( new QSignalMapper(this))
          , _sm_str( new QSignalMapper(this))
          , _sm_folder( new QSignalMapper(this))
          , _sm_spin( new QSignalMapper(this))
        {
          if( _s.status() != QSettings::NoError)
          {
            printf("Error creating QSettings\n");
          }
          
          _map.insert( "prefix",
                       Item( "pixmaps", FolderDialog));
          _map.insert( "suffix",
                       Item( "png", LineEdit));
          _map.insert( "show_key",
                       Item( 0, SpinBox, new QIntValidator(-1, 2, this)));
          _map.insert( "icon_font",
                       Item( QFont(QFont().family(), 6), FontDialog));
          _map.insert( "splash_font",
                       Item( QFont(), FontDialog));
          initGui();
        }
  
  void initGui()
        {
          QFormLayout* l = new QFormLayout;
          for(QMap<QString,Item>::iterator it = _map.begin();
              it != _map.end(); ++it)
          {
            QVariant v = _s.value(it.key(), it.value().data);
            _s.setValue( it.key(), v);
            it.value().data = v;
                        
            QWidget* buddy=0;
            switch( it.value().hint)
            {
            case( FontDialog):
            {
              QFont f( v.value<QFont>());
              QPushButton* pb = new QPushButton(
                  f.family() + " " + QString::number(f.pointSize()), this);
              pb->setFont( f);
              buddy = pb;
              connect( buddy, SIGNAL( clicked()), _sm_font, SLOT(map()));
              _sm_font->setMapping( buddy, it.key());
            }
            break;
            case( FolderDialog):
            {
              QString f( v.toString());
              QPushButton* pb = new QPushButton( f, this);
              buddy = pb;
              connect( buddy, SIGNAL( clicked()), _sm_folder, SLOT(map()));
              _sm_folder->setMapping( buddy, it.key());
            }
            break;
            case( SpinBox):
            {
              QSpinBox* sb = new QSpinBox( this);
              if( it.value().valid)
              {
                QIntValidator* valid =
                    static_cast<QIntValidator*>( it.value().valid);
                sb->setRange( valid->bottom(),valid->top());
              }
              sb->setValue( v.toInt());
              buddy = sb;
              connect( buddy, SIGNAL( valueChanged ( int )),
                       _sm_spin, SLOT(map()));
              _sm_spin->setMapping( buddy, it.key());
            }
            break;
            case( LineEdit):
            default:
            {
              QLineEdit* le = new QLineEdit( v.toString(), this);
              if( it.value().valid)
                  le->setValidator(it.value().valid);
              buddy = le;
              connect( buddy, SIGNAL( textEdited ( const QString& )),
                       _sm_str, SLOT(map()));
              _sm_str->setMapping( buddy, it.key());
            }
            break;
            }
          
            l->addRow( new QLabel(it.key(), this), buddy);
          }
          connect( _sm_font, SIGNAL( mapped( const QString&)),
                   this, SLOT(chooseFont( const QString&)));
          connect( _sm_str, SIGNAL( mapped( const QString&)),
                   this, SLOT(saveStr( const QString&)));
          connect( _sm_folder, SIGNAL( mapped( const QString&)),
                   this, SLOT(chooseFolder( const QString&)));
          connect( _sm_spin, SIGNAL( mapped( const QString&)),
                   this, SLOT( saveInt( const QString&)));
          
          setLayout(l);
        }

  void setValue( const QString& key, const QVariant& val)
        {
          QMap<QString,Item>::iterator it = _map.find( key);
          if( it != _map.end())
          {
            it.value().data = val;
            _s.setValue( key, val);
            switch( it.value().hint)
            {
            case( FontDialog):
            {
              QPushButton* pb =
                  static_cast<QPushButton*>( _sm_font->mapping(key));
              QFont font(val.value<QFont>());
              pb->setText( font.family() + " " + QString::number(font.pointSize()));
              pb->setFont( font);
            }
            break;
            case( FolderDialog):
            {
              QPushButton* pb =
                  static_cast<QPushButton*>( _sm_folder->mapping(key));
              pb->setText( val.toString());
            }
            break;
            case( SpinBox):
            {
              QSpinBox* sb =
                  static_cast<QSpinBox*>( _sm_spin->mapping(key));
              sb->setValue( val.toInt());
            }
            break;
            case( LineEdit):
            default:
            {
              QLineEdit* le =
                  static_cast<QLineEdit*>( _sm_str->mapping(key));
              le->setText( val.toString());
            }
            break;
            }
          }
        }
  
  QVariant value( const QString& key) const
        {
          QMap<QString,Item>::const_iterator it = _map.find( key);
          if( it != _map.end())
          {
            return it.value().data;
          }
          return QVariant();
        }
        

private slots:
  void chooseFont( const QString& key)
        {
          QFont current( _s.value(key).value<QFont>());
          
          bool ok;
          QFont font(QFontDialog::getFont( &ok, current, this));
          if(!ok) return;
          setValue( key, font);
          emit update();
        }
  
  void chooseFolder( const QString& key)
        {
          QString current( _s.value(key).toString());
          
          QString dirname = QFileDialog::getExistingDirectory( 0, key, current);
          if( dirname.isEmpty() || dirname == current) return;
          setValue( key, dirname);
          emit update();
        }
  
  void saveStr( const QString& key)
        {
          QLineEdit* le = static_cast<QLineEdit*>(_sm_str->mapping( key));
          setValue( key, le->text());
          emit update();
        }

  void saveInt( const QString& key)
        {
          QSpinBox* sb = static_cast<QSpinBox*>(_sm_spin->mapping( key));
          setValue( key, sb->value());
          emit update();
        }

signals:
  void update();
  
private:
  QSettings _s;
  QSignalMapper* _sm_font;
  QSignalMapper* _sm_str;
  QSignalMapper* _sm_folder;
  QSignalMapper* _sm_spin;
  QMap<QString,Item> _map;
};


#endif
