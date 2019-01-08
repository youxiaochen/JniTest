package you.jnitest;

/**
 * Created by you on 2018/12/18.
 */

public class JniTest {

    public final String a;

    public final String b;

    public JniTest(String a, String b) {
        this.a = a;
        this.b = b;
    }

    static {
        System.loadLibrary("project");
    }

    /**
     * 创建JniTest对象
     * @param a
     * @param b
     * @return
     */
    public static native JniTest createBean(String a, String b);

    /**
     * 获取app的sha1值
     * @return
     */
    public static native String getSha1();

}
