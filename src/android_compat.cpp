#include "android_compat.h"

#if defined (PLATFORM_ANDROID)

void requestPermissions(){
    QStringList permissions = {
        "android.permission.READ_MEDIA_AUDIO",
        "android.permission.WRITE_MEDIA_AUDIO"
    };

    for (const QString &perm : permissions) {
        QtAndroid::requestPermissionsSync(QStringList() << perm);
    }
}

/*
*
* AI generated as android permissions in c++ are too convoluted
*
*/

int createFileAndroid(string fileName, string relativePath, void* dataBuf, uint64_t dataBufSize){
    string type = fName.substr(fName.rfind('.') + 1, fName.npos);
    QString fileType;

    switch (type) {
    case "mp3":
        fileType = "audio/mpeg";
        break;
    case "flac":
        fileType = "audio/flac";
        break;
    default:
        return 1;
    }

    QAndroidJniEnvironment env;
    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject resolver = activity.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");

    // Get MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
    QAndroidJniObject mediaUri = QAndroidJniObject::getStaticObjectField(
        "android/provider/MediaStore$Audio$Media",
        "EXTERNAL_CONTENT_URI",
        "Landroid/net/Uri;");

    QAndroidJniObject values("android/content/ContentValues", "()V");

    QAndroidJniObject colDisplayName = QAndroidJniObject::getStaticObjectField(
        "android/provider/MediaStore$MediaColumns",
        "DISPLAY_NAME",
        "Ljava/lang/String;");

    QAndroidJniObject colMimeType = QAndroidJniObject::getStaticObjectField(
        "android/provider/MediaStore$MediaColumns",
        "MIME_TYPE",
        "Ljava/lang/String;");

    QAndroidJniObject colRelativePath = QAndroidJniObject::getStaticObjectField(
        "android/provider/MediaStore$MediaColumns",
        "RELATIVE_PATH",
        "Ljava/lang/String;");

    values.callMethod<void>("put",
                            "(Ljava/lang/String;Ljava/lang/String;)V",
                            colDisplayName.object<jstring>(),
                            QAndroidJniObject::fromString(QString::fromStdString(fileName)).object<jstring>());

    values.callMethod<void>("put",
                            "(Ljava/lang/String;Ljava/lang/String;)V",
                            colMimeType.object<jstring>(),
                            QAndroidJniObject::fromString(fileType).object<jstring>());

    values.callMethod<void>("put",
                            "(Ljava/lang/String;Ljava/lang/String;)V",
                            colRelativePath.object<jstring>(),
                            QAndroidJniObject::fromString(QString::fromStdString(string("Music/" + relativePath))).object<jstring>());

    // Insert new item into MediaStore
    QAndroidJniObject audioUri = resolver.callObjectMethod(
        "insert",
        "(Landroid/net/Uri;Landroid/content/ContentValues;)Landroid/net/Uri;",
        mediaUri.object<jobject>(),
        values.object<jobject>());

    if (!audioUri.isValid()) {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return 1;
    }

    // Open output stream
    QAndroidJniObject outStream = resolver.callObjectMethod(
        "openOutputStream",
        "(Landroid/net/Uri;)Ljava/io/OutputStream;",
        audioUri.object<jobject>());

    if (!outStream.isValid()) {
        return 2;
    }

    // Write dummy data
    QByteArray data = QByteArray(static_cast<char*>(dataBuf), dataBufSize);
    jbyteArray jdata = env->NewByteArray(data.size());
    env->SetByteArrayRegion(jdata, 0, data.size(), reinterpret_cast<const jbyte*>(data.constData()));
    outStream.callMethod<void>("write", "([B)V", jdata);
    outStream.callMethod<void>("close", "()V");

    return 0;
}

#endif
