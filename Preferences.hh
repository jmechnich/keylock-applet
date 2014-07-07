#ifndef PREFERENCES_HH
#define PREFERENCES_HH

#include <QWidget>
#include <QIntValidator>
#include <QSignalMapper>
#include <QFontDialog>
#include <QFileDialog>
#include <QSpinBox>
#include <QLineEdit>
#include <QSettings>
#include <QComboBox>

class Preferences : public QWidget
{
  Q_OBJECT

public:

  enum GuiHint
  {
      SpinBox,
      LineEdit,
      FontDialog,
      FolderDialog,
      ComboBox
  };
  
  struct Item
  {
    Item( const QVariant& d=QVariant(), GuiHint h=LineEdit, QValidator* v=0)
            : data(d)
            , hint(h)
            , valid(v)
            , vals()
          {}

    Item( const QVariant& d, GuiHint h, const QString& v)
            : data(d)
            , hint(h)
            , valid(0)
            , vals(v)
          {}
    
    QVariant data;
    GuiHint  hint;
    QValidator* valid;
    QString  vals;
  };

  Preferences();
  
  void initGui();
  
  void setValue( const QString& key, const QVariant& val);
    
  QVariant value( const QString& key) const
        {
          map_const_iterator it = _map.find( key);
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

  void saveStrChoice( const QString& key)
        {
          QComboBox* cb = static_cast<QComboBox*>(_sm_combo->mapping( key));
          setValue( key, cb->currentText());
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
  typedef QMap<QString,Item> map_type;
  typedef map_type::const_iterator map_const_iterator;
  typedef map_type::iterator map_iterator;
  map_type _map;

  QSettings _s;
  QSignalMapper* _sm_font;
  QSignalMapper* _sm_str;
  QSignalMapper* _sm_folder;
  QSignalMapper* _sm_spin;
  QSignalMapper* _sm_combo;
};


#endif
