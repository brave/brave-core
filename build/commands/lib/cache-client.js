const { pipeline } = require("stream");
const { promisify } = require("util");
const pipe = promisify(pipeline);

const createS3Client = async () => {
  const {
    AWS_REGION: region, 
    BRAVE_TEST_CACHE_S3_BUCKET: bucket
  } = process.env;

  if (!region || !bucket) 
    return null;

  const { 
    S3Client, 
    HeadObjectCommand, 
    GetObjectCommand
  } = require( "@aws-sdk/client-s3");
  const s3 = new S3Client({region: AWS_REGION });

  return {
    async check(key) {
      return s3.send(new HeadObjectCommand({
        Bucket: bucket, 
        Key: key 
      })).then(()=>true).catch(()=>false)
    },
    
    async download(key, dest) {
      const response = await s3.send(new GetObjectCommand({
        Bucket: bucket,
        Key: key
      }));

      try {
        await pipe(response.Body, createWriteStream(dest));

        console.log(`Downloaded s3://${bucket}/${key} to ${dest}`);
        return true;
      } catch (e) {
        console.error(e);
        return false;
      }
    },

    async upload(key, source) {
      
      const fileStream = fs.createReadStream(source);

      const uploadParams = {
        Bucket: bucket,
        Key: key,
        Body: fileStream,
        ContentType: "text/plain",
      };

      await s3.send(new PutObjectCommand(uploadParams));

      console.log(`Uploaded ${file} to s3://${bucket}/${key}`);
      return true;
    }
  }
}

const createFsClient = async () => {
  const {
    BRAVE_TEST_CACHE_PATH: cacheDir
  } = process.env;

  if (cacheDir)
    return null;

  await fs.mkdirp(cacheDir)

  return {
    async check(key) {
      const localFile = path.join(cacheDir, key); 
      return fs.exists(localFile);
    },
    
    async download(key, dest) {
      const localFile = path.join(cacheDir, key); 
      return fs.copy(localFile, dest)
        .then(()=>true)
        .catch(()=>false);
    },

    async upload(key, source) {
      const localFile = path.join(cacheDir, key); 
      return fs.copy(source, localFile)
        .then(()=>true)
        .catch(()=>false);
    }
  }
}


const cacheClient = async () => {

  const [fsClient, s3Client] = await Promise.all(
    [createFsClient(), createS3Client()]
  );

  if (!fsClient && !s3Client) 
    return null;

  return {
    async check(key) {
      return (
        await fsClient?.check(key) || 
        await s3Client?.check(key)
      );
    },

    async download(key, dest) {
      return (
        await fsClient?.download(key, dest) ||
        await s3Client?.download(key, dest)
      );
    },

    async upload(key, upload) {
      return Promise.all([
        fsClient?.upload(key, upload),
        s3Client?.upload(key, upload),
      ]).then( ([fs, s3]) =>  fs && s3);
    }
  }
  
}

module.exports = cacheClient;