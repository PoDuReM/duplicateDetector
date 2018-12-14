#include <searcher.h>

const qint64 READK = 30;

bool Searcher::checkStop() {
    return QThread::currentThread()->isInterruptionRequested();
}

void Searcher::get_duplicates(QString const &dir) {
    QMap<qint64, QVector<QString>> size2Files;

    QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
    qint64 view_size = 0, all_size = 0;
    qint8 percent = 1;
    auto changed_progress = [&view_size, &all_size, &percent]() {
        qint8 new_percent = static_cast<qint8>(view_size * 100 / all_size);
        if (new_percent > percent) {
            percent = new_percent;
            return true;
        }
        return false;
    };
    while (it.hasNext()) {
        QString path = it.next();
        qint64 sz = QFileInfo(path).size();
        size2Files[sz].push_back(path);
        all_size += sz;
        if (checkStop()){
            emit finish();
            return;
        }
    }
    for (auto itSize = size2Files.begin(); itSize != size2Files.end(); ++itSize) {
        if (itSize.value().size() == 1) {
            view_size += itSize.key();
            if (changed_progress()) {
                emit progress(percent);
            }
            continue;
        }
        if (checkStop()) {
            emit finish();
            return;
        }
        QMap<QString, QVector<QString>> first2Files;
        for (auto file : itSize.value()) {
            first2Files[get_first_k(file, std::min(itSize.key(), READK))].push_back(file);
        }
        for (auto itFirst = first2Files.begin(); itFirst != first2Files.end(); ++itFirst) {
            if (itFirst.value().size() == 1) {
                view_size += itSize.key();
                if (changed_progress()) {
                    emit progress(percent);
                }
                continue;
            }
            if (checkStop()) {
                emit finish();
                return;
            }
            QMap<QByteArray, QVector<QString>> hash2Files;
            for (auto file : itFirst.value()) {
                hash2Files[get_hash(file)].push_back(file);
            }
            for (auto itHash = hash2Files.begin(); itHash != hash2Files.end(); ++itHash) {
                if (itHash.value().size() == 1) {
                    view_size += itSize.key();
                    if (changed_progress()) {
                        emit progress(percent);
                    }
                    continue;
                }
                if (checkStop()) {
                    emit finish();
                    return;
                }
                emit send_duplicates(itHash.value());
                view_size += itSize.key() * itHash.value().size();
                if (changed_progress()) {
                    emit progress(percent);
                }
            }
        }
    }
    emit finish();
}

QByteArray Searcher::get_hash(const QString &filepath) {
    QCryptographicHash md5(QCryptographicHash::Md5);
    QFile file(filepath);
    file.open(QIODevice::ReadOnly);
    md5.addData(file.readAll());
    return md5.result();
}

QString Searcher::get_first_k(QString const& filepath, qint64 x){
    QFile file(filepath);
    file.open(QIODevice::ReadOnly);
    return file.read(x);
}