#include "Preferences.hh"

#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

Preferences::Preferences() 
        : QWidget()
        , _s(this)
        , _sm_font(new QSignalMapper(this))
        , _sm_str(new QSignalMapper(this))
        , _sm_folder(new QSignalMapper(this))
        , _sm_spin(new QSignalMapper(this))
        , _sm_combo(new QSignalMapper(this))
{
  if (_s.status() != QSettings::NoError)
      printf("Error creating QSettings\n");

  _map.insert("prefix", Item("icons", FolderDialog));
  _map.insert("suffix", Item("png", ComboBox, "png,svg"));
  _map.insert("show_key", Item(0, SpinBox, new QIntValidator(0, 3, this)));
  _map.insert("icon_font", Item(QFont(QFont().family(), 6), FontDialog));
  _map.insert("splash_font", Item(QFont(), FontDialog));
  initGui();
}

void
Preferences::initGui()
{
  QFormLayout* l = new QFormLayout;
  for (map_iterator it = _map.begin(); it != _map.end(); ++it)
  {
    QVariant v = _s.value(it.key(), it.value().data);
    _s.setValue(it.key(), v);
    it.value().data = v;

    QWidget* buddy=0;
    switch (it.value().hint)
    {
    case FontDialog:
    {
      QFont f(v.value<QFont>());
      QPushButton* pb = new QPushButton(
          f.family() + " " + QString::number(f.pointSize()), this);
      pb->setFont(f);
      buddy = pb;
      connect(buddy, SIGNAL(clicked()), _sm_font, SLOT(map()));
      _sm_font->setMapping(buddy, it.key());
    }
    break;
    case FolderDialog:
    {
      QString f(v.toString());
      QPushButton* pb = new QPushButton(f, this);
      buddy = pb;
      connect(buddy, SIGNAL(clicked()), _sm_folder, SLOT(map()));
      _sm_folder->setMapping(buddy, it.key());
    }
    break;
    case SpinBox:
    {
      QSpinBox* sb = new QSpinBox(this);
      if (it.value().valid)
      {
        QIntValidator* valid =
            static_cast<QIntValidator*>(it.value().valid);
        sb->setRange(valid->bottom(),valid->top());
      }
      sb->setValue(v.toInt());
      buddy = sb;
      connect(buddy, SIGNAL(valueChanged(int)),
              _sm_spin, SLOT(map()));
      _sm_spin->setMapping(buddy, it.key());
    }
    break;
    case ComboBox:
    {
      QComboBox* cb = new QComboBox(this);
      QStringList sl(it.value().vals.split(','));
      for (int i=0; i<sl.size(); ++i)
          cb->addItem(sl.at(i));
      cb->setCurrentIndex(cb->findText(v.toString()));
      buddy = cb;
      connect(buddy, SIGNAL(activated(const QString&)),
              _sm_combo, SLOT(map()));
      _sm_combo->setMapping(buddy, it.key());
    }
    break;
    case LineEdit:
    default:
    {
      QLineEdit* le = new QLineEdit(v.toString(), this);
      if (it.value().valid)
          le->setValidator(it.value().valid);
      buddy = le;
      connect(buddy, SIGNAL(textEdited(const QString&)),
              _sm_str, SLOT(map()));
      _sm_str->setMapping(buddy, it.key());
    }
    break;
    }

    l->addRow(new QLabel(it.key(), this), buddy);
  }
  connect(_sm_font, SIGNAL(mapped(const QString&)),
          this, SLOT(chooseFont(const QString&)));
  connect(_sm_str, SIGNAL(mapped(const QString&)),
          this, SLOT(saveStr(const QString&)));
  connect(_sm_folder, SIGNAL(mapped(const QString&)),
          this, SLOT(chooseFolder(const QString&)));
  connect(_sm_spin, SIGNAL(mapped(const QString&)),
          this, SLOT(saveInt(const QString&)));
  connect(_sm_combo, SIGNAL(mapped(const QString&)),
          this, SLOT(saveStrChoice(const QString&)));

  setLayout(l);
}

void
Preferences::setValue(const QString& key, const QVariant& val)
{
  QMap<QString,Item>::iterator it = _map.find(key);
  if (it != _map.end())
  {
    it.value().data = val;
    _s.setValue(key, val);
    switch (it.value().hint)
    {
    case FontDialog:
    {
      QPushButton* pb = static_cast<QPushButton*>(_sm_font->mapping(key));
      QFont font(val.value<QFont>());
      pb->setText(font.family() + " " + QString::number(font.pointSize()));
      pb->setFont(font);
    }
    break;
    case FolderDialog:
    {
      QPushButton* pb = static_cast<QPushButton*>(_sm_folder->mapping(key));
      pb->setText(val.toString());
    }
    break;
    case SpinBox:
    {
      QSpinBox* sb = static_cast<QSpinBox*>(_sm_spin->mapping(key));
      sb->setValue(val.toInt());
    }
    break;
    case ComboBox:
    {
      QComboBox* cb = static_cast<QComboBox*>(_sm_combo->mapping(key));
      cb->setCurrentIndex(cb->findText(val.toString()));
    }
    break;
    case LineEdit:
    default:
    {
      QLineEdit* le = static_cast<QLineEdit*>(_sm_str->mapping(key));
      le->setText(val.toString());
    }
    break;
    }
  }
}
