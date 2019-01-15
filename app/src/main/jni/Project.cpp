#include <jni.h>
#include <assert.h>
#include<android/log.h>

// 这个是自定义的LOG的标识
#define TAG    "jniLog"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
 
#ifdef __cplusplus

extern "C"{
    /**
     * 转16进制字符
     */
    const char hc[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    //类成员
    jclass contextClass;
    jclass signatureClass;
    jclass packageNameClass;
    jclass packageInfoClass;

    /**
    * 获取Context对象
    * @param env
    * @return
    */
    jobject context(JNIEnv *env) {
        //获取Activity Thread的实例对象
        jclass activityThread = env->FindClass("android/app/ActivityThread");
        jmethodID currentActivityThread = env->GetStaticMethodID(activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
        jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);
        //获取Application，也就是全局的Context
        jmethodID getApplication = env->GetMethodID(activityThread, "getApplication", "()Landroid/app/Application;");
        jobject context = env->CallObjectMethod(at, getApplication);
        env->DeleteLocalRef(activityThread);
        env->DeleteLocalRef(at);
        return context;
    }

    __attribute__((section (".test")))
    JNIEXPORT jobject JNICALL crb(JNIEnv *env, jclass obj, jstring a, jstring b) {
        jclass jniTestClass = env->FindClass("you/jnitest/JniTest");
        jmethodID methodId = env->GetMethodID(jniTestClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
        return  env->NewObject(jniTestClass, methodId, a, b);
    }

    __attribute__((section (".test")))
    JNIEXPORT jstring JNICALL sha(JNIEnv *env, jclass obj) {
        jobject contextObject = context(env);
        //反射获取PackageManager
        jmethodID methodId = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject package_manager = env->CallObjectMethod(contextObject, methodId);
        if (package_manager == NULL) {
            return NULL;
        }
        //反射获取包名
        methodId = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
        jstring package_name = (jstring)env->CallObjectMethod(contextObject, methodId);
        if (package_name == NULL) {
            return NULL;
        }
        //获取PackageInfo对象
        methodId = env->GetMethodID(packageNameClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
        jobject package_info = env->CallObjectMethod(package_manager, methodId, package_name, 0x40);
        env->DeleteLocalRef(package_manager);

        if (package_info == NULL) {
            return NULL;
        }
        //获取签名信息
        jfieldID fieldId = env->GetFieldID(packageInfoClass, "signatures", "[Landroid/content/pm/Signature;");
        jobjectArray signature_object_array = (jobjectArray)env->GetObjectField(package_info, fieldId);
        env->DeleteLocalRef(package_info);
        if (signature_object_array == NULL) {
            return NULL;
        }
        jobject signature_object = env->GetObjectArrayElement(signature_object_array, 0);
        env->DeleteLocalRef(signature_object_array);

        //签名信息转换成sha1值
        jclass signature_class = env->GetObjectClass(signature_object);
        methodId = env->GetMethodID(signature_class, "toByteArray", "()[B");
        env->DeleteLocalRef(signature_class);

        jbyteArray signature_byte = (jbyteArray) env->CallObjectMethod(signature_object, methodId);
        env->DeleteLocalRef(signature_object);

        jclass byte_array_input_class = env->FindClass("java/io/ByteArrayInputStream");
        methodId = env->GetMethodID(byte_array_input_class, "<init>", "([B)V");
        jobject byte_array_input = env->NewObject(byte_array_input_class, methodId, signature_byte);
        env->DeleteLocalRef(byte_array_input_class);
        env->DeleteLocalRef(signature_byte);

        jclass certificate_factory_class = env->FindClass("java/security/cert/CertificateFactory");
        methodId = env->GetStaticMethodID(certificate_factory_class,"getInstance","(Ljava/lang/String;)Ljava/security/cert/CertificateFactory;");
        jstring x_509_jstring = env->NewStringUTF("X.509");
        jobject cert_factory = env->CallStaticObjectMethod(certificate_factory_class,methodId,x_509_jstring);
        methodId = env->GetMethodID(certificate_factory_class,"generateCertificate",("(Ljava/io/InputStream;)Ljava/security/cert/Certificate;"));
        jobject x509_cert = env->CallObjectMethod(cert_factory,methodId, byte_array_input);
        env->DeleteLocalRef(certificate_factory_class);
        env->DeleteLocalRef(cert_factory);

        jclass x509_cert_class = env->GetObjectClass(x509_cert);
        methodId = env->GetMethodID(x509_cert_class,"getEncoded", "()[B");
        jbyteArray cert_byte = (jbyteArray)env->CallObjectMethod(x509_cert, methodId);
        env->DeleteLocalRef(x509_cert_class);
        env->DeleteLocalRef(x509_cert);

        jclass message_digest_class = env->FindClass("java/security/MessageDigest");
        methodId = env->GetStaticMethodID(message_digest_class, "getInstance", "(Ljava/lang/String;)Ljava/security/MessageDigest;");
        jstring sha1_jstring = env->NewStringUTF("SHA1");
        jobject sha1_digest = env->CallStaticObjectMethod(message_digest_class,methodId,sha1_jstring);
        methodId = env->GetMethodID(message_digest_class,"digest","([B)[B");
        jbyteArray sha1_byte = (jbyteArray)env->CallObjectMethod(sha1_digest, methodId, cert_byte);
        env->DeleteLocalRef(message_digest_class);
        env->DeleteLocalRef(sha1_digest);
        env->DeleteLocalRef(cert_byte);

        //转换成char
        jsize array_size = env->GetArrayLength(sha1_byte);
        jbyte* sha1 = env->GetByteArrayElements(sha1_byte,NULL);
        char *hex_sha = new char[array_size*2 + 1];
        for (int i = 0; i <array_size ; ++i) {
            hex_sha[2*i] = hc[((unsigned char)sha1[i])/16];
            hex_sha[2*i+1] = hc[((unsigned char)sha1[i])%16];
        }
        hex_sha[array_size * 2]='\0';
        env->ReleaseByteArrayElements(sha1_byte, sha1, 0);
        return  (env)->NewStringUTF(hex_sha);
    }

    /* 通过JNI动态注册代替静态注册
    * JNI函数混淆去掉Java_包名_方法名的写法, 如  Java_you_jnitest_JniTest_createBean方法名注册为crb, 方法参数不能变
    * 方法数组中第一参数为需要混淆的方法 如 createBean, 第二个为方法参数与返回值JNI的写法, 第三个即为混淆时注册的方法名,对应上面的方法
    */
    JNINativeMethod gMethods[] = {
            { "createBean", "(Ljava/lang/String;Ljava/lang/String;)Lyou/jnitest/JniTest;", (void*)crb},
            {"getSha1", "()Ljava/lang/String;", (void*)sha},
    };

    /*
    * Register native methods for all classes we know about.
    */
    int registerNatives(JNIEnv* env, jclass clazz) {
        if ((env)->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
            return JNI_FALSE;
        }
        return JNI_TRUE;
    }

    JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM* vm,void* reserved) {
        JNIEnv* env = NULL;
        jint result = -1;
        if ((vm)->GetEnv( (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
            return -1;
        }
        assert(env != NULL);
        //需要注册的类
        jclass jniTestClass = env->FindClass("you/jnitest/JniTest");
        assert(jniTestClass != NULL);
        if (!registerNatives(env, jniTestClass)) {//注册
            return -1;
        }
        contextClass = (jclass)env->NewGlobalRef((env)->FindClass("android/content/Context"));
        signatureClass = (jclass)env->NewGlobalRef((env)->FindClass("android/content/pm/Signature"));
        packageNameClass = (jclass)env->NewGlobalRef((env)->FindClass("android/content/pm/PackageManager"));
        packageInfoClass = (jclass)env->NewGlobalRef((env)->FindClass("android/content/pm/PackageInfo"));
        return JNI_VERSION_1_4;
    }

}
#endif
